#pragma once
#include <volk/volk.h>
#include <array>
#include <vector>
#include "core/vulkanContext.h"

namespace caldera {

template <uint32_t FramesInFlight>
struct SyncObjects {
  std::array<VkFence, FramesInFlight> frameFences{};
  std::array<VkSemaphore, FramesInFlight> presentSemaphores{};
  std::vector<VkSemaphore> renderSemaphores;  // one per swapchain image

  void init(VulkanContext& ctx, uint32_t swapchainImageCount) {
    VkSemaphoreCreateInfo semCI{.sType =
                                  VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    for (uint32_t i = 0; i < FramesInFlight; i++) {
      vkCheck(vkCreateFence(ctx.device, &fenceCI, nullptr, &frameFences[i]));
      vkCheck(
        vkCreateSemaphore(ctx.device, &semCI, nullptr, &presentSemaphores[i]));
    }

    renderSemaphores.resize(swapchainImageCount);
    for (auto& sem : renderSemaphores) {
      vkCheck(vkCreateSemaphore(ctx.device, &semCI, nullptr, &sem));
    }
  }

  void waitAndResetFence(VkDevice device, uint32_t frameIndex) {
    vkCheck(vkWaitForFences(device, 1, &frameFences[frameIndex], VK_TRUE,
                            UINT64_MAX));
    vkCheck(vkResetFences(device, 1, &frameFences[frameIndex]));
  }

  void destroy(VkDevice device) {
    for (uint32_t i = 0; i < FramesInFlight; i++) {
      vkDestroyFence(device, frameFences[i], nullptr);
      vkDestroySemaphore(device, presentSemaphores[i], nullptr);
    }
    for (auto& sem : renderSemaphores) {
      vkDestroySemaphore(device, sem, nullptr);
    }
  }
};

}  // namespace caldera
