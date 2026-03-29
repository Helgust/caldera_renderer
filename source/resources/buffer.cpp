#include "resources/buffer.h"
#include "core/vulkanContext.h"

namespace caldera {

Buffer Buffer::createHostVisible(VmaAllocator allocator, VkDeviceSize size,
                                 VkBufferUsageFlags usage, VkDevice device) {
  Buffer buf{};

  VkBufferCreateInfo bufCI{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                           .size = size,
                           .usage = usage};

  VmaAllocationCreateInfo allocCI{
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
             VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
             VMA_ALLOCATION_CREATE_MAPPED_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO};

  vkCheck(vmaCreateBuffer(allocator, &bufCI, &allocCI, &buf.handle,
                          &buf.allocation, &buf.info));

  if (device != VK_NULL_HANDLE &&
      (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)) {
    VkBufferDeviceAddressInfo bdaInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = buf.handle};
    buf.deviceAddress = vkGetBufferDeviceAddress(device, &bdaInfo);
  }

  return buf;
}

Buffer Buffer::createGpuOnly(VmaAllocator allocator, VkDeviceSize size,
                             VkBufferUsageFlags usage) {
  Buffer buf{};

  VkBufferCreateInfo bufCI{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                           .size = size,
                           .usage = usage};

  VmaAllocationCreateInfo allocCI{
    .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO};

  vkCheck(vmaCreateBuffer(allocator, &bufCI, &allocCI, &buf.handle,
                          &buf.allocation, &buf.info));
  return buf;
}

}  // namespace caldera
