#pragma once
#include <volk/volk.h>
#include <cstdint>

// Thin VK_EXT_debug_utils wrappers: name GPU objects and annotate command
// buffer regions so RenderDoc / validation output is readable. Every call is
// gated on gDebugUtilsEnabled, set once in VulkanContext::init — when the
// extension isn't present (typically release builds) these compile to a
// branch-and-return.

namespace caldera {

inline bool gDebugUtilsEnabled = false;

// Attach a human-readable name to any Vulkan handle. `type` must match the
// handle (e.g. VK_OBJECT_TYPE_IMAGE for a VkImage).
inline void setObjectName(VkDevice device, uint64_t handle, VkObjectType type,
                          const char* name) {
  if (!gDebugUtilsEnabled || !name)
    return;
  const VkDebugUtilsObjectNameInfoEXT info{
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
    .objectType = type,
    .objectHandle = handle,
    .pObjectName = name};
  vkSetDebugUtilsObjectNameEXT(device, &info);
}

inline void beginDebugLabel(VkCommandBuffer cb, const char* name, float r,
                            float g, float b) {
  if (!gDebugUtilsEnabled)
    return;
  const VkDebugUtilsLabelEXT label{
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
    .pLabelName = name,
    .color = {r, g, b, 1.0f}};
  vkCmdBeginDebugUtilsLabelEXT(cb, &label);
}

inline void endDebugLabel(VkCommandBuffer cb) {
  if (!gDebugUtilsEnabled)
    return;
  vkCmdEndDebugUtilsLabelEXT(cb);
}

// Deterministic, well-spread color from a label string so each pass gets a
// stable, distinct hue in the RenderDoc event browser.
inline void labelColor(const char* name, float& r, float& g, float& b) {
  uint32_t h = 2166136261u;  // FNV-1a
  for (const char* p = name; p && *p; ++p)
    h = (h ^ static_cast<uint8_t>(*p)) * 16777619u;
  r = 0.35f + ((h >> 0) & 0xFF) / 255.0f * 0.6f;
  g = 0.35f + ((h >> 8) & 0xFF) / 255.0f * 0.6f;
  b = 0.35f + ((h >> 16) & 0xFF) / 255.0f * 0.6f;
}

// RAII command-buffer region: label opens on construction, closes on scope
// exit. Use for the lifetime of a pass's recorded commands.
struct DebugLabelScope {
  VkCommandBuffer cb;

  DebugLabelScope(VkCommandBuffer cb, const char* name) : cb(cb) {
    float r, g, b;
    labelColor(name, r, g, b);
    beginDebugLabel(cb, name, r, g, b);
  }

  ~DebugLabelScope() { endDebugLabel(cb); }

  DebugLabelScope(const DebugLabelScope&) = delete;
  DebugLabelScope& operator=(const DebugLabelScope&) = delete;
};

}  // namespace caldera
