#pragma once
#include <volk/volk.h>
#include <glm/glm.hpp>
#include <vector>
#include "core/vulkanContext.h"

namespace caldera {

struct Swapchain {
  VkSwapchainKHR handle{VK_NULL_HANDLE};
  std::vector<VkImage> images;
  std::vector<VkImageView> views;
  VkFormat format{VK_FORMAT_B8G8R8A8_SRGB};
  VkExtent2D extent{};

  void init(VulkanContext& ctx, VkSurfaceKHR surface, glm::ivec2 size);
  void recreate(VulkanContext& ctx, VkSurfaceKHR surface, glm::ivec2 size);
  void destroy(VkDevice device);

 private:
  void createImageViews(VkDevice device);
  VkSwapchainCreateInfoKHR lastCI_{};
};

}  // namespace caldera
