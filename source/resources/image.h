#pragma once
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>

namespace caldera {

struct Image {
  VkImage handle{VK_NULL_HANDLE};
  VkImageView view{VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
  VkFormat format{VK_FORMAT_UNDEFINED};
  VkExtent3D extent{};

  static Image create2D(VmaAllocator allocator, VkDevice device,
                        VkFormat format, VkExtent2D extent,
                        VkImageUsageFlags usage, VkImageAspectFlags aspect);

  // Inserts a VkImageMemoryBarrier2 into cb.
  // The frame graph will call this — same method, computed inputs.
  void transitionLayout(VkCommandBuffer cb, VkImageLayout oldLayout,
                        VkImageLayout newLayout, VkPipelineStageFlags2 srcStage,
                        VkAccessFlags2 srcAccess,
                        VkPipelineStageFlags2 dstStage,
                        VkAccessFlags2 dstAccess,
                        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);

  void destroy(VkDevice device, VmaAllocator allocator);
};

// Texture = Image + sampler, used for material bindings
struct Texture {
  Image image;
  VkSampler sampler{VK_NULL_HANDLE};

  void destroy(VkDevice device, VmaAllocator allocator);
};

}  // namespace caldera
