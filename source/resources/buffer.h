#pragma once
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>
#include <cstring>
#include "core/assert.h"

namespace caldera {

struct Buffer {
  VkBuffer handle{VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
  VmaAllocationInfo info{};
  VkDeviceAddress deviceAddress{0};

  // Host-visible, persistently mapped.
  // Pass VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT to also fill deviceAddress.
  static Buffer createHostVisible(VmaAllocator allocator, VkDeviceSize size,
                                  VkBufferUsageFlags usage,
                                  VkDevice device = VK_NULL_HANDLE);

  static Buffer createGpuOnly(VmaAllocator allocator, VkDeviceSize size,
                              VkBufferUsageFlags usage);

  void upload(const void* data, VkDeviceSize size) {
    // Tripwire: a null mapping means the allocation didn't land in
    // host-visible memory (see createHostVisible), so this memcpy would
    // fault. Fail loudly here instead of crashing inside memcpy.
    CALDERA_ASSERT(info.pMappedData);
    memcpy(info.pMappedData, data, static_cast<size_t>(size));
  }

  void* mapped() { return info.pMappedData; }

  void destroy(VmaAllocator allocator) {
    vmaDestroyBuffer(allocator, handle, allocation);
    handle = VK_NULL_HANDLE;
    allocation = VK_NULL_HANDLE;
    deviceAddress = 0;
  }
};

}  // namespace caldera
