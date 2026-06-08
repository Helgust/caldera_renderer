#pragma once
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>
#include <vulkan/vk_enum_string_helper.h>
#include <cstdio>
#include <source_location>
#include <string>

namespace caldera {

static inline void vkCheck(
  VkResult r, std::source_location loc = std::source_location::current()) {
  if (r == VK_SUCCESS) {
    return;
  }
  std::string loc_info =
    std::string(loc.file_name()) + ":" + std::to_string(loc.line());
  fprintf(stderr, "Vulkan result: code(%d) - '%s' <%s>\n", (int)r,
          string_VkResult(r), loc_info.c_str());
  if (r != VK_SUCCESS) {
    fprintf(stderr, "Vulkan aborting:");
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
  VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};

  void init();
  void destroy();
};

}  // namespace caldera
