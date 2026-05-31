#pragma once
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>
#include <cstdint>
#include <functional>
#include <vector>

#include "resources/image.h"

namespace caldera {

struct VulkanContext;
class GpuTimer;

using FgResource = uint32_t;  // index into FrameGraph::resources_

enum class FgUsage {
  COLOR_ATTACHMENT,
  DEPTH_ATTACHMENT,
  SAMPLED_READ,  // shader read (e.g. read a prior pass output)
  PRESENT        // swapchain → PRESENT_SRC
};

// Per-access load/store intent. Only meaningful when the access is an
// attachment (Color/Depth); ignored for SAMPLED_READ/PRESENT. The graph
// overrides `load` to CLEAR on a resource's first-ever write, since
// LOAD-ing never-written contents is undefined.
struct FgAttachmentOp {
  VkAttachmentLoadOp load = VK_ATTACHMENT_LOAD_OP_CLEAR;
  VkAttachmentStoreOp store = VK_ATTACHMENT_STORE_OP_STORE;
  VkClearValue clearValue{.color{{0.0f, 0.0f, 0.0f, 1.0f}}};
};

// One resource touched by a pass, plus how the pass uses it. `op` is only
// read for attachment usages.
struct FgAccess {
  FgResource resource;
  FgUsage usage;
  FgAttachmentOp op{};
};

struct FgResourceDesc {
  const char* name;
  VkFormat format;
  VkExtent2D extent;
  VkImageUsageFlags usage;
  VkImageAspectFlags aspect;
  bool imported = false;  // true = swapchain image, graph doesn't allocate
};

struct FgPass {
  const char* name;
  std::vector<FgAccess> accesses;
  std::function<void(VkCommandBuffer)> execute;  // record draws only
};

// Runtime sync state, tracked so the graph can derive the next barrier
// from the resource's previous usage.
struct FgResourceState {
  VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
  VkPipelineStageFlags2 stage{VK_PIPELINE_STAGE_2_NONE};
  VkAccessFlags2 access{VK_ACCESS_2_NONE};
};

// Simple linear frame graph: passes run in declaration order on a single
// queue. The graph owns transient images, auto-inserts barriers from each
// resource's last-known state, and drives dynamic rendering. No aliasing,
// reordering, or async compute (Phase 3 extensions).
class FrameGraph {
 public:
  explicit FrameGraph(VulkanContext& ctx);
  ~FrameGraph();

  FrameGraph(const FrameGraph&) = delete;
  FrameGraph& operator=(const FrameGraph&) = delete;

  // Externally-owned image (swapchain). The graph never allocates or
  // destroys it. Re-import each frame with the acquired image index.
  FgResource importImage(const char* name, VkImage handle, VkImageView view,
                         VkFormat format, VkExtent2D extent);

  // Transient image. Allocated immediately, destroyed by reset()/dtor.
  FgResource createImage(const FgResourceDesc& desc);

  void addPass(const char* name, std::vector<FgAccess> accesses,
               std::function<void(VkCommandBuffer)> execute);

  void execute(VkCommandBuffer cb, GpuTimer* timer = nullptr);

  // Frees graph-owned images and clears passes. Call before rebuilding
  // (e.g. swapchain resize) or destruction.
  void reset();

  const Image& image(FgResource r) const { return resources_[r].image; }

 private:
  struct Resource {
    FgResourceDesc desc;
    Image image;
    FgResourceState state;
    bool owned;           // true = graph allocated, must destroy
    bool written{false};  // any pass has stored to it → LOAD is now valid
  };

  void beginRendering(VkCommandBuffer cb, const FgPass& pass, bool& out_began);

  VulkanContext& ctx_;
  std::vector<Resource> resources_;
  std::vector<FgPass> passes_;
};

}  // namespace caldera
