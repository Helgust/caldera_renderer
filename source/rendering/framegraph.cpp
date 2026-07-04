#include "rendering/framegraph.h"

#include <array>

#include "core/debugUtils.h"
#include "core/gpuTimer.h"
#include "core/vulkanContext.h"

namespace caldera {

// FgUsage → the (layout, stage, access) a resource must be in for that use.
// This table is the graph's job: it replaces the hand-written barriers
// that used to live inline in the render loop.
static FgResourceState requiredState(FgUsage u) {
  switch (u) {
    case FgUsage::COLOR_ATTACHMENT:
      return {VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
              VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
              VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT};
    case FgUsage::DEPTH_ATTACHMENT:
      return {VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
              VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT};
    case FgUsage::SAMPLED_READ:
      return {VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
              VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
              VK_ACCESS_2_SHADER_SAMPLED_READ_BIT};
    case FgUsage::PRESENT:
      return {VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
              VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
              VK_ACCESS_2_NONE};
  }
  return {};
}

// Barrier subresource aspect must cover stencil too for combined formats.
static VkImageAspectFlags barrierAspect(VkFormat f) {
  switch (f) {
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
      return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D16_UNORM:
      return VK_IMAGE_ASPECT_DEPTH_BIT;
    default:
      return VK_IMAGE_ASPECT_COLOR_BIT;
  }
}

FrameGraph::FrameGraph(VulkanContext& ctx) : ctx_(ctx) {}

FrameGraph::~FrameGraph() {
  reset();
}

FgResource FrameGraph::importImage(const char* name, VkImage handle,
                                   VkImageView view, VkFormat format,
                                   VkExtent2D extent) {
  Resource r{};
  r.desc = {.name = name,
            .format = format,
            .extent = extent,
            .usage = 0,
            .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
            .imported = true};
  r.image.handle = handle;
  r.image.view = view;
  r.image.format = format;
  r.image.extent = {extent.width, extent.height, 1};
  r.state = {};  // swapchain image is effectively UNDEFINED after acquire
  r.owned = false;
  resources_.push_back(std::move(r));
  setObjectName(ctx_.device, (uint64_t)handle, VK_OBJECT_TYPE_IMAGE, name);
  setObjectName(ctx_.device, (uint64_t)view, VK_OBJECT_TYPE_IMAGE_VIEW, name);
  return static_cast<FgResource>(resources_.size() - 1);
}

FgResource FrameGraph::createImage(const FgResourceDesc& desc) {
  Resource r{};
  r.desc = desc;
  r.desc.imported = false;
  r.image = Image::create2D(ctx_.allocator, ctx_.device, desc.format,
                            desc.extent, desc.usage, desc.aspect);
  r.state = {};
  r.owned = true;
  resources_.push_back(std::move(r));
  // Read the image back out of the stored resource, not `r`: it was just
  // moved from. (Works today only because Image is trivially copyable.)
  const Resource& added = resources_.back();
  setObjectName(ctx_.device, (uint64_t)added.image.handle, VK_OBJECT_TYPE_IMAGE,
                desc.name);
  setObjectName(ctx_.device, (uint64_t)added.image.view,
                VK_OBJECT_TYPE_IMAGE_VIEW, desc.name);
  return static_cast<FgResource>(resources_.size() - 1);
}

void FrameGraph::addPass(const char* name, std::vector<FgAccess> accesses,
                         std::function<void(VkCommandBuffer)> execute) {
  passes_.push_back({.name = name,
                     .accesses = std::move(accesses),
                     .execute = std::move(execute)});
}

void FrameGraph::beginRendering(VkCommandBuffer cb, const FgPass& pass,
                                bool& out_began) {
  std::array<VkRenderingAttachmentInfo, 8> colors{};
  uint32_t colorCount{0};
  VkRenderingAttachmentInfo depth{};
  bool hasDepth{false};
  VkExtent2D area{};

  for (const FgAccess& a : pass.accesses) {
    Resource& r = resources_[a.resource];
    if (a.usage != FgUsage::COLOR_ATTACHMENT &&
        a.usage != FgUsage::DEPTH_ATTACHMENT)
      continue;

    // Honour the caller's requested load/store, except: LOAD-ing a
    // resource nothing has written yet is undefined, so the first write
    // is forced to CLEAR regardless of what the pass asked for.
    VkAttachmentLoadOp loadOp = a.op.load;
    if (!r.written && loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
      loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    const VkRenderingAttachmentInfo att{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = r.image.view,
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .loadOp = loadOp,
      .storeOp = a.op.store,
      .clearValue = a.op.clearValue};

    if (a.usage == FgUsage::COLOR_ATTACHMENT) {
      colors[colorCount++] = att;
      area = r.desc.extent;
    } else {
      depth = att;
      hasDepth = true;
      if (colorCount == 0)
        area = r.desc.extent;
    }

    // A STORE makes the contents valid for a later LOAD; DONT_CARE
    // leaves them undefined, so don't promise LOAD will work.
    if (a.op.store == VK_ATTACHMENT_STORE_OP_STORE)
      r.written = true;
  }

  if (colorCount == 0 && !hasDepth) {
    out_began = false;
    return;  // e.g. a present-only pass — barrier work, no rendering
  }

  VkRenderingInfo info{
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea{.extent = area},
    .layerCount = 1,
    .colorAttachmentCount = colorCount,
    .pColorAttachments = colorCount ? colors.data() : nullptr,
    .pDepthAttachment = hasDepth ? &depth : nullptr};
  vkCmdBeginRendering(cb, &info);
  out_began = true;
}

void FrameGraph::execute(VkCommandBuffer cb, GpuTimer* timer) {
  for (const FgPass& pass : passes_) {
    DebugLabelScope _label(cb, pass.name);
    if (timer)
      timer->beginPass(cb, pass.name);

    // 1. Transition each accessed resource from its last-known state.
    for (const FgAccess& a : pass.accesses) {
      Resource& r = resources_[a.resource];
      const FgResourceState want = requiredState(a.usage);
      const FgResourceState cur = r.state;
      r.image.transitionLayout(cb, cur.layout, want.layout, cur.stage,
                               cur.access, want.stage, want.access,
                               barrierAspect(r.desc.format));
      r.state = want;
    }

    // 2. Drive dynamic rendering for attachment passes.
    bool began{false};
    beginRendering(cb, pass, began);

    // 3. User records draws.
    if (pass.execute)
      pass.execute(cb);

    // 4. Close the render scope.
    if (began)
      vkCmdEndRendering(cb);

    if (timer)
      timer->endPass(cb);
  }
}

void FrameGraph::reset() {
  for (Resource& r : resources_) {
    // Tripwire. The graph is loop-local, so ~FrameGraph -> reset() runs right
    // after submit, before the frame fence proves the GPU is done. Destroying
    // a graph-owned (transient) image here is a use-after-free. It is safe
    // today only because every resource is imported (owned == false). Before
    // createImage() is used for real, the graph needs GPU-lifetime tracking
    // (a per-frame deletion queue, or hoist the graph out of the render loop).
    CALDERA_ASSERT_MSG(!r.owned,
                       "FrameGraph owns transient image '%s' but has no "
                       "GPU-lifetime tracking; destroying it here races the "
                       "in-flight frame",
                       r.desc.name);
    if (r.owned)
      r.image.destroy(ctx_.device, ctx_.allocator);
  }
  resources_.clear();
  passes_.clear();
}

}  // namespace caldera
