#include "resources/image.h"
#include "core/vulkanContext.h"

namespace caldera {

Image Image::create2D(VmaAllocator allocator, VkDevice device, VkFormat format,
                      VkExtent2D extent2D, VkImageUsageFlags usage,
                      VkImageAspectFlags aspect) {
  Image img{};
  img.format = format;
  img.extent = {extent2D.width, extent2D.height, 1};

  VkImageCreateInfo imageCI{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                            .imageType = VK_IMAGE_TYPE_2D,
                            .format = format,
                            .extent = img.extent,
                            .mipLevels = 1,
                            .arrayLayers = 1,
                            .samples = VK_SAMPLE_COUNT_1_BIT,
                            .tiling = VK_IMAGE_TILING_OPTIMAL,
                            .usage = usage,
                            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

  VmaAllocationCreateInfo allocCI{
    .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO};

  vkCheck(vmaCreateImage(allocator, &imageCI, &allocCI, &img.handle,
                         &img.allocation, nullptr));

  VkImageViewCreateInfo viewCI{
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = img.handle,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = format,
    .subresourceRange{.aspectMask = aspect, .levelCount = 1, .layerCount = 1}};
  vkCheck(vkCreateImageView(device, &viewCI, nullptr, &img.view));

  return img;
}

void Image::transitionLayout(VkCommandBuffer cb, VkImageLayout old_layout,
                             VkImageLayout new_layout,
                             VkPipelineStageFlags2 src_stage,
                             VkAccessFlags2 src_access,
                             VkPipelineStageFlags2 dst_stage,
                             VkAccessFlags2 dst_access,
                             VkImageAspectFlags aspect) {
  VkImageMemoryBarrier2 barrier{
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask = src_stage,
    .srcAccessMask = src_access,
    .dstStageMask = dst_stage,
    .dstAccessMask = dst_access,
    .oldLayout = old_layout,
    .newLayout = new_layout,
    .image = handle,
    .subresourceRange = {
      .aspectMask = aspect, .levelCount = 1, .layerCount = 1}};

  VkDependencyInfo dep{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                       .imageMemoryBarrierCount = 1,
                       .pImageMemoryBarriers = &barrier};
  vkCmdPipelineBarrier2(cb, &dep);
}

void Image::destroy(VkDevice device, VmaAllocator allocator) {
  vkDestroyImageView(device, view, nullptr);
  vmaDestroyImage(allocator, handle, allocation);
  view = VK_NULL_HANDLE;
  handle = VK_NULL_HANDLE;
}

void Texture::destroy(VkDevice device, VmaAllocator allocator) {
  vkDestroySampler(device, sampler, nullptr);
  image.destroy(device, allocator);
}

}  // namespace caldera
