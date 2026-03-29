#include "core/swapchain.h"
#include <cassert>

namespace caldera {

void Swapchain::init(VulkanContext& ctx, VkSurfaceKHR surface,
                     glm::ivec2 size) {
  format = VK_FORMAT_B8G8R8A8_SRGB;

  VkSurfaceCapabilitiesKHR caps{};
  vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, surface,
                                                    &caps));
  extent = {static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)};

  lastCI_ = VkSwapchainCreateInfoKHR{
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface,
    .minImageCount = caps.minImageCount,
    .imageFormat = format,
    .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR};

  vkCheck(vkCreateSwapchainKHR(ctx.device, &lastCI_, nullptr, &handle));

  uint32_t imageCount{0};
  vkCheck(vkGetSwapchainImagesKHR(ctx.device, handle, &imageCount, nullptr));
  images.resize(imageCount);
  vkCheck(
    vkGetSwapchainImagesKHR(ctx.device, handle, &imageCount, images.data()));

  createImageViews(ctx.device);
}

void Swapchain::recreate(VulkanContext& ctx, VkSurfaceKHR surface,
                         glm::ivec2 size) {
  for (auto& view : views) {
    vkDestroyImageView(ctx.device, view, nullptr);
  }

  VkSurfaceCapabilitiesKHR caps{};
  vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, surface,
                                                    &caps));
  extent = {static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)};

  lastCI_.oldSwapchain = handle;
  lastCI_.imageExtent = extent;

  VkSwapchainKHR newSwapchain{VK_NULL_HANDLE};
  vkCheck(vkCreateSwapchainKHR(ctx.device, &lastCI_, nullptr, &newSwapchain));
  vkDestroySwapchainKHR(ctx.device, lastCI_.oldSwapchain, nullptr);
  handle = newSwapchain;
  lastCI_.oldSwapchain = VK_NULL_HANDLE;

  uint32_t imageCount{0};
  vkCheck(vkGetSwapchainImagesKHR(ctx.device, handle, &imageCount, nullptr));
  images.resize(imageCount);
  vkCheck(
    vkGetSwapchainImagesKHR(ctx.device, handle, &imageCount, images.data()));

  createImageViews(ctx.device);
}

void Swapchain::createImageViews(VkDevice device) {
  views.resize(images.size());
  for (size_t i = 0; i < images.size(); i++) {
    VkImageViewCreateInfo viewCI{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .levelCount = 1,
                        .layerCount = 1}};
    vkCheck(vkCreateImageView(device, &viewCI, nullptr, &views[i]));
  }
}

void Swapchain::destroy(VkDevice device) {
  for (auto& view : views) {
    vkDestroyImageView(device, view, nullptr);
  }
  vkDestroySwapchainKHR(device, handle, nullptr);
}

}  // namespace caldera
