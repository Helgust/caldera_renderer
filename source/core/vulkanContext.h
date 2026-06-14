#pragma once
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>
#include <vulkan/vk_enum_string_helper.h>
#include <source_location>
#include "core/assert.h"

namespace caldera {

// Same idea as the book's `check(result)` macro — log "file(line) : message"
// then break on failure — but a function so the argument (usually an inline
// Vulkan call like vkCheck(vkCreate...())) is evaluated exactly once. The
// source_location keeps the logged file/line pointing at the real call site.
static inline void vkCheck(
  VkResult r, std::source_location loc = std::source_location::current()) {
  if (r == VK_SUCCESS)
    return;
  logMessage("%s(%u) : Vulkan error %d '%s'\n", loc.file_name(),
             (unsigned)loc.line(), (int)r, string_VkResult(r));
  onCheckFailed(string_VkResult(r));
  CALDERA_DEBUG_BREAK();
}

static inline void sdlCheck(
  bool ok, std::source_location loc = std::source_location::current()) {
  if (ok)
    return;
  logMessage("%s(%u) : SDL error\n", loc.file_name(), (unsigned)loc.line());
  onCheckFailed("SDL error");
  CALDERA_DEBUG_BREAK();
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
