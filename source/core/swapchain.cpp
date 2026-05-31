#include "core/swapchain.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace caldera {

void Swapchain::init(VulkanContext& ctx, VkSurfaceKHR surface,
                     glm::ivec2 size) {
  const std::vector<VkFormat> surfaceImageFormats = {
    VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM,
    VK_FORMAT_R8G8B8_UNORM};
  const VkColorSpaceKHR surfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

  std::vector<VkSurfaceFormatKHR> supportedFormats;
  uint32_t supportedCount{0};

  vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, surface,
                                               &supportedCount, NULL));
  supportedFormats.resize(supportedCount);
  vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
    ctx.physicalDevice, surface, &supportedCount, supportedFormats.data()));

  bool formatFound = false;
  const uint32_t surfaceFormatCount = surfaceImageFormats.size();

  for (int i = 0; i < surfaceFormatCount; i++) {
    for (int j = 0; j < supportedCount; j++) {
      if (supportedFormats[j].format == surfaceImageFormats[i] &&
          supportedFormats[j].colorSpace == surfaceColorSpace) {
        format = supportedFormats[j].format;
        formatFound = true;
        break;
      }
    }

    if (formatFound)
      break;
  }

  // Default to the first format supported.
  if (!formatFound) {
    format = supportedFormats[0].format;
    assert(false);
  }

  VkSurfaceCapabilitiesKHR caps{};
  vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, surface,
                                                    &caps));

  extent = caps.currentExtent;
  if (extent.width == UINT32_MAX) {
    extent.width =
      glm::clamp(static_cast<uint32_t>(size.x), caps.minImageExtent.width,
                 caps.maxImageExtent.width);
    extent.height =
      glm::clamp(static_cast<uint32_t>(size.y), caps.minImageExtent.height,
                 caps.maxImageExtent.height);
  }

  fprintf(stdout, "Create swapchain %u %u - saved %u %u, min image %u\n",
          extent.width, extent.height, size.x, size.y, caps.minImageCount);
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
  extent = caps.currentExtent;
  if (extent.width == UINT32_MAX) {
    extent.width =
      glm::clamp(static_cast<uint32_t>(size.x), caps.minImageExtent.width,
                 caps.maxImageExtent.width);
    extent.height =
      glm::clamp(static_cast<uint32_t>(size.y), caps.minImageExtent.height,
                 caps.maxImageExtent.height);
  }

  fprintf(stdout, "Recreate swapchain %u %u - saved %u %u, min image %u\n",
          extent.width, extent.height, size.x, size.y, caps.minImageCount);

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
