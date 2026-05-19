#include "ui/imguiLayer.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "core/vulkanContext.h"

namespace caldera {

void ImGuiLayer::init(VulkanContext& ctx, SDL_Window* window, VkQueue queue,
                      uint32_t queueFamily, VkFormat colorFormat,
                      uint32_t minImageCount, uint32_t imageCount) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui_ImplSDL3_InitForVulkan(window);

  // Dynamic rendering: ImGui needs the color format(s) it will draw into
  // instead of a VkRenderPass.
  VkPipelineRenderingCreateInfo renderingCI{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &colorFormat};

  ImGui_ImplVulkan_InitInfo info{};
  info.ApiVersion = VK_API_VERSION_1_3;
  info.Instance = ctx.instance;
  info.PhysicalDevice = ctx.physicalDevice;
  info.Device = ctx.device;
  info.QueueFamily = queueFamily;
  info.Queue = queue;
  info.MinImageCount = minImageCount;
  info.ImageCount = imageCount;
  // Let the backend create + own its descriptor pool (size > 0).
  info.DescriptorPoolSize = 16;
  info.UseDynamicRendering = true;
  info.PipelineInfoMain.PipelineRenderingCreateInfo = renderingCI;
  info.CheckVkResultFn = vkCheck;

  ImGui_ImplVulkan_Init(&info);

  initialized_ = true;
}

void ImGuiLayer::destroy(VulkanContext& ctx) {
  if (!initialized_)
    return;
  vkCheck(vkDeviceWaitIdle(ctx.device));
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  initialized_ = false;
}

bool ImGuiLayer::processEvent(const SDL_Event& e) {
  ImGui_ImplSDL3_ProcessEvent(&e);
  return wantCaptureMouse();
}

bool ImGuiLayer::wantCaptureMouse() const {
  return initialized_ && ImGui::GetIO().WantCaptureMouse;
}

void ImGuiLayer::beginFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::draw(VkCommandBuffer cb) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
}

}  // namespace caldera
