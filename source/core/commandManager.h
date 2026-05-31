#pragma once
#include <volk/volk.h>
#include <array>
#include <functional>
#include "core/vulkanContext.h"

namespace caldera {

using RecordFn = std::function<void(VkCommandBuffer)>;

template <uint32_t FramesInFlight>
struct CommandManager {
  VkCommandPool pool{VK_NULL_HANDLE};
  std::array<VkCommandBuffer, FramesInFlight> buffers{};

  void init(VulkanContext& ctx) {
    VkCommandPoolCreateInfo poolCI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = ctx.graphicsFamily};
    vkCheck(vkCreateCommandPool(ctx.device, &poolCI, nullptr, &pool));

    VkCommandBufferAllocateInfo cbAllocCI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .commandBufferCount = FramesInFlight};
    vkCheck(vkAllocateCommandBuffers(ctx.device, &cbAllocCI, buffers.data()));
  }

  // Allocates a one-time CB, records via fn, submits and blocks until done.
  // Use only at init time (texture uploads, layout transitions, etc.)
  void submitOneTime(VulkanContext& ctx, RecordFn fn) {
    VkCommandBufferAllocateInfo cbAI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .commandBufferCount = 1};
    VkCommandBuffer cb{};
    vkCheck(vkAllocateCommandBuffers(ctx.device, &cbAI, &cb));

    VkCommandBufferBeginInfo beginBI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkCheck(vkBeginCommandBuffer(cb, &beginBI));
    fn(cb);
    vkCheck(vkEndCommandBuffer(cb));

    VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence{};
    vkCheck(vkCreateFence(ctx.device, &fenceCI, nullptr, &fence));

    VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                            .commandBufferCount = 1,
                            .pCommandBuffers = &cb};
    vkCheck(vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, fence));
    vkCheck(vkWaitForFences(ctx.device, 1, &fence, VK_TRUE, UINT64_MAX));

    vkDestroyFence(ctx.device, fence, nullptr);
    vkFreeCommandBuffers(ctx.device, pool, 1, &cb);
  }

  VkCommandBuffer& get(uint32_t frame_index) { return buffers[frame_index]; }

  void destroy(VkDevice device) { vkDestroyCommandPool(device, pool, nullptr); }
};

}  // namespace caldera
