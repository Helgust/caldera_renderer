#pragma once
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>
#include <vector>

namespace caldera {

// Thin check helpers — used by every module
static inline void vkCheck(VkResult r) {
  if (r != VK_SUCCESS) {
    fprintf(stderr, "Vulkan error: %d\n", r);
    exit(r);
  }
}

static inline void sdlCheck(bool ok) {
  if (!ok) {
    fprintf(stderr, "SDL error\n");
    exit(1);
  }
}

struct VulkanContext {
  VkInstance instance{VK_NULL_HANDLE};
  VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
  VkDevice device{VK_NULL_HANDLE};
  VkQueue graphicsQueue{VK_NULL_HANDLE};
  uint32_t graphicsFamily{0};
  VmaAllocator allocator{VK_NULL_HANDLE};

  void init(uint32_t deviceIndex = 0);
  void destroy();
};

}  // namespace caldera
