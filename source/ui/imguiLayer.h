#pragma once
#include <volk/volk.h>

struct SDL_Window;
union SDL_Event;

namespace caldera {

struct VulkanContext;

// Thin owner around the Dear ImGui SDL3 + Vulkan backends.
//
// Rendering uses dynamic rendering (no VkRenderPass), matching the rest of
// the engine. Record ImGui inside an already-open rendering scope on the
// backbuffer (see FrameGraph "forward" pass) by calling draw(cb).
class ImGuiLayer {
 public:
  void init(VulkanContext& ctx, SDL_Window* window, VkQueue queue,
            uint32_t queueFamily, VkFormat colorFormat, uint32_t minImageCount,
            uint32_t imageCount);
  void destroy(VulkanContext& ctx);

  // Feed an SDL event to ImGui. Returns true if ImGui wants to consume the
  // mouse/keyboard so the caller can skip its own camera/input handling.
  bool processEvent(const SDL_Event& e);
  bool wantCaptureMouse() const;

  // Start a new UI frame. Build your widgets between this and draw().
  void beginFrame();

  // Finalize the frame and record draw commands. Must be called while a
  // dynamic-rendering scope targeting the backbuffer is open.
  void draw(VkCommandBuffer cb);

 private:
  bool initialized_{false};
};

}  // namespace caldera
