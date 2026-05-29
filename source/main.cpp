#define VOLK_IMPLEMENTATION
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk/volk.h>
#include <vulkan/vulkan.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <string_view>

#include "core/commandManager.h"
#include "core/debugUtils.h"
#include "core/gpuTimer.h"
#include "core/swapchain.h"
#include "core/syncObjects.h"
#include "core/vulkanContext.h"
#include "rendering/framegraph.h"
#include "rendering/pipeline.h"
#include "rendering/shader.h"
#include "resources/buffer.h"
#include "resources/image.h"
#include "scene/sceneLoader.h"
#include "ui/imguiLayer.h"

#include <imgui.h>

using namespace caldera;

// -----------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------
constexpr uint32_t kFramesInFlight{2};

// -----------------------------------------------------------------------
// Per-frame shader data (passed via BDA push constant)
// -----------------------------------------------------------------------
struct ShaderData {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model[3];
  glm::vec4 lightPos{0.0f, -10.0f, 10.0f, 0.0f};
  uint32_t selected{1};
};

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
static VkFormat findDepthFormat(VkPhysicalDevice physDevice) {
  for (VkFormat fmt :
       {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}) {
    VkFormatProperties2 props{.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
    vkGetPhysicalDeviceFormatProperties2(physDevice, fmt, &props);
    if (props.formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
      return fmt;
  }
  return VK_FORMAT_UNDEFINED;
}

static void uploadTexture(VulkanContext& ctx,
                          CommandManager<kFramesInFlight>& cmdManager,
                          Texture& tex, const ImageData& img) {
  // Staging buffer
  Buffer staging = Buffer::createHostVisible(ctx.allocator, img.pixels.size(),
                                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  staging.upload(img.pixels.data(), img.pixels.size());

  // GPU image
  tex.image = Image::create2D(
    ctx.allocator, ctx.device, VK_FORMAT_R8G8B8A8_SRGB,
    {static_cast<uint32_t>(img.width), static_cast<uint32_t>(img.height)},
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_IMAGE_ASPECT_COLOR_BIT);

  cmdManager.submitOneTime(ctx, [&](VkCommandBuffer cb) {
    tex.image.transitionLayout(
      cb, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

    VkBufferImageCopy region{
      .imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .layerCount = 1},
      .imageExtent{static_cast<uint32_t>(img.width),
                   static_cast<uint32_t>(img.height), 1}};
    vkCmdCopyBufferToImage(cb, staging.handle, tex.image.handle,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    tex.image.transitionLayout(
      cb, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_ACCESS_SHADER_READ_BIT);
  });

  staging.destroy(ctx.allocator);

  VkSamplerCreateInfo samplerCI{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                .magFilter = VK_FILTER_LINEAR,
                                .minFilter = VK_FILTER_LINEAR,
                                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                .anisotropyEnable = VK_TRUE,
                                .maxAnisotropy = 8.0f,
                                .maxLod = 1.0f};
  vkCheck(vkCreateSampler(ctx.device, &samplerCI, nullptr, &tex.sampler));
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------
static void printUsage(const char* exe) {
  SDL_Log("Usage: %s [--device <index>] [--scene <gltf-path>]", exe);
}

int main(int argc, char* argv[]) {
  sdlCheck(SDL_Init(SDL_INIT_VIDEO));
  sdlCheck(SDL_Vulkan_LoadLibrary(NULL));

  uint32_t deviceIndex = 0;
  std::string scenePath =
    std::string(ASSET_PATH) + "scenes/damaged-helmet/damaged-helmet.gltf";

  for (int i = 1; i < argc; ++i) {
    std::string_view arg{argv[i]};
    if ((arg == "--device" || arg == "-d") && i + 1 < argc) {
      deviceIndex = static_cast<uint32_t>(std::stoi(argv[++i]));
    } else if ((arg == "--scene" || arg == "-s") && i + 1 < argc) {
      scenePath = argv[++i];
    } else {
      SDL_Log("Unknown argument: %s", argv[i]);
      printUsage(argv[0]);
    }
  }

  // --- Core ---
  VulkanContext ctx;
  ctx.init(deviceIndex);

  SDL_Window* window = SDL_CreateWindow(
    "Caldera Renderer", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  assert(window);

  VkSurfaceKHR surface{VK_NULL_HANDLE};
  sdlCheck(SDL_Vulkan_CreateSurface(window, ctx.instance, nullptr, &surface));

  glm::ivec2 windowSize{1280, 720};

  Swapchain swapchain;
  swapchain.init(ctx, surface, windowSize);

  CommandManager<kFramesInFlight> cmdManager;
  cmdManager.init(ctx);

  SyncObjects<kFramesInFlight> sync;
  sync.init(ctx, static_cast<uint32_t>(swapchain.images.size()));

  // --- Depth image ---
  const VkFormat depthFormat = findDepthFormat(ctx.physicalDevice);
  assert(depthFormat != VK_FORMAT_UNDEFINED);

  std::array<Image, kFramesInFlight> depthImages;
  for (size_t i = 0; i < depthImages.size(); ++i) {
    depthImages[i] = Image::create2D(
      ctx.allocator, ctx.device, depthFormat,
      {static_cast<uint32_t>(windowSize.x),
       static_cast<uint32_t>(windowSize.y)},
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    std::string depthName = "depth[" + std::to_string(i) + "]";
    setObjectName(ctx.device, (uint64_t)depthImages[i].handle,
                  VK_OBJECT_TYPE_IMAGE, depthName.c_str());
    setObjectName(ctx.device, (uint64_t)depthImages[i].view,
                  VK_OBJECT_TYPE_IMAGE_VIEW, depthName.c_str());
  }
  // --- Scene ---
  SceneLoader sceneLoader;
  Scene scene = sceneLoader.load(scenePath);

  const auto& vertices = scene.mesh.vertices;
  const auto& indices = scene.mesh.indices;
  VkDeviceSize vBufSize = sizeof(Vertex) * vertices.size();
  VkDeviceSize iBufSize = sizeof(uint16_t) * indices.size();

  Buffer geometryBuffer = Buffer::createHostVisible(
    ctx.allocator, vBufSize + iBufSize,
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  setObjectName(ctx.device, (uint64_t)geometryBuffer.handle,
                VK_OBJECT_TYPE_BUFFER, "geometry");

  geometryBuffer.upload(vertices.data(), vBufSize);
  memcpy(static_cast<char*>(geometryBuffer.mapped()) + vBufSize, indices.data(),
         iBufSize);

  // --- Textures ---
  std::vector<Texture> textures(scene.images.size());
  std::vector<VkDescriptorImageInfo> textureDescriptors;
  for (size_t i = 0; i < scene.images.size(); i++) {
    uploadTexture(ctx, cmdManager, textures[i], scene.images[i]);
    textureDescriptors.push_back(
      {.sampler = textures[i].sampler,
       .imageView = textures[i].image.view,
       .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL});
  }

  // --- Shader data buffers (per frame, BDA) ---
  std::array<Buffer, kFramesInFlight> shaderDataBuffers;
  for (size_t i = 0; i < shaderDataBuffers.size(); ++i) {
    shaderDataBuffers[i] = Buffer::createHostVisible(
      ctx.allocator, sizeof(ShaderData),
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, ctx.device);
    std::string n = "shaderData[" + std::to_string(i) + "]";
    setObjectName(ctx.device, (uint64_t)shaderDataBuffers[i].handle,
                  VK_OBJECT_TYPE_BUFFER, n.c_str());
  }

  // --- Descriptors ---
  VkDescriptorBindingFlags descVariableFlag{
    VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT};
  VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    .bindingCount = 1,
    .pBindingFlags = &descVariableFlag};
  VkDescriptorSetLayoutBinding descBinding{
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = static_cast<uint32_t>(textures.size()),
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT};
  VkDescriptorSetLayoutCreateInfo descLayoutCI{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = &descBindingFlags,
    .bindingCount = 1,
    .pBindings = &descBinding};
  VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
  vkCheck(vkCreateDescriptorSetLayout(ctx.device, &descLayoutCI, nullptr,
                                      &descriptorSetLayout));

  VkDescriptorPoolSize poolSize{
    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = static_cast<uint32_t>(textures.size())};
  VkDescriptorPoolCreateInfo poolCI{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = 1,
    .poolSizeCount = 1,
    .pPoolSizes = &poolSize};
  VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
  vkCheck(
    vkCreateDescriptorPool(ctx.device, &poolCI, nullptr, &descriptorPool));

  uint32_t variableDescCount{static_cast<uint32_t>(textures.size())};
  VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountAI{
    .sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
    .descriptorSetCount = 1,
    .pDescriptorCounts = &variableDescCount};
  VkDescriptorSetAllocateInfo descSetAlloc{
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = &variableCountAI,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = 1,
    .pSetLayouts = &descriptorSetLayout};
  VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
  vkCheck(vkAllocateDescriptorSets(ctx.device, &descSetAlloc, &descriptorSet));

  VkWriteDescriptorSet writeDesc{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = descriptorSet,
    .dstBinding = 0,
    .descriptorCount = static_cast<uint32_t>(textureDescriptors.size()),
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo = textureDescriptors.data()};
  vkUpdateDescriptorSets(ctx.device, 1, &writeDesc, 0, nullptr);

  // --- Shader + Pipeline ---
  SlangCompiler slang;
  slang.init();

  ShaderModule shaderModule = ShaderModule::loadSlang(
    ctx.device, slang.session.get(), std::string(SHADER_PATH) + "shader.slang",
    "triangle");

  VkPushConstantRange pushConstant{.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                                   .size = sizeof(VkDeviceAddress)};
  VkPipelineLayoutCreateInfo layoutCI{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &descriptorSetLayout,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges = &pushConstant};
  VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  vkCheck(
    vkCreatePipelineLayout(ctx.device, &layoutCI, nullptr, &pipelineLayout));

  VkVertexInputBindingDescription vertexBinding{
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
  std::vector<VkVertexInputAttributeDescription> vertexAttribs{
    {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT},
    {.location = 1,
     .binding = 0,
     .format = VK_FORMAT_R32G32B32_SFLOAT,
     .offset = offsetof(Vertex, normal)},
    {.location = 2,
     .binding = 0,
     .format = VK_FORMAT_R32G32_SFLOAT,
     .offset = offsetof(Vertex, uv)}};

  VkPipeline pipeline = GraphicsPipelineBuilder{}
                          .setShaderStages(shaderModule.handle)
                          .setVertexInput(vertexBinding, vertexAttribs)
                          .setDepthStencil(true, true)
                          .setColorAttachmentFormat(swapchain.format)
                          .setDepthAttachmentFormat(depthFormat)
                          .setPipelineLayout(pipelineLayout)
                          .build(ctx.device);

  // --- GPU profiler ---
  constexpr uint32_t kMaxPassesPerFrame{16};
  GpuTimer gpuTimer;
  gpuTimer.init(ctx, kFramesInFlight, kMaxPassesPerFrame);

  // --- ImGui ---
  ImGuiLayer ui;
  ui.init(ctx, window, ctx.graphicsQueue, ctx.graphicsFamily, swapchain.format,
          static_cast<uint32_t>(swapchain.images.size()),
          static_cast<uint32_t>(swapchain.images.size()));

  // --- State ---
  ShaderData shaderData{};
  glm::vec3 camPos{0.0f, 0.0f, -6.0f};
  glm::vec3 objectRotations[3]{};
  uint32_t frameInFlightIndex{0};
  uint32_t swapchainImageIndex{0};
  bool updateSwapchain{false};
  bool quit{false};
  uint64_t lastTime{SDL_GetTicks()};

  // --- Render loop ---
  while (!quit) {
    sync.waitAndResetFence(ctx.device, frameInFlightIndex);

    VkResult acquireResult =
      vkAcquireNextImageKHR(ctx.device, swapchain.handle, UINT64_MAX,
                            sync.presentSemaphores[frameInFlightIndex],
                            VK_NULL_HANDLE, &swapchainImageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
      updateSwapchain = true;
    } else {
      vkCheck(acquireResult);
    }

    // Update transforms
    shaderData.projection =
      glm::perspective(glm::radians(45.0f),
                       (float)windowSize.x / (float)windowSize.y, 0.1f, 32.0f);
    shaderData.view = glm::translate(glm::mat4(1.0f), camPos);
    for (int i = 0; i < 3; i++) {
      auto instancePos = glm::vec3((float)(i - 1) * 3.0f, 0.0f, 0.0f);
      shaderData.model[i] = glm::translate(glm::mat4(1.0f), instancePos) *
                            glm::mat4_cast(glm::quat(objectRotations[i]));
    }
    shaderDataBuffers[frameInFlightIndex].upload(&shaderData,
                                                 sizeof(ShaderData));

    // --- Build UI ---
    ui.beginFrame();
    ImGui::Begin("Caldera");
    ImGui::Text("%.1f FPS (%.2f ms)", ImGui::GetIO().Framerate,
                1000.0f / ImGui::GetIO().Framerate);
    int selected = static_cast<int>(shaderData.selected);
    if (ImGui::SliderInt("Selected mesh", &selected, 0, 2))
      shaderData.selected = static_cast<uint32_t>(selected);
    ImGui::SliderFloat("Camera Z", &camPos.z, -16.0f, -1.0f);
    ImGui::DragFloat3("Light pos", &shaderData.lightPos.x, 0.1f);
    ImGui::End();

    ImGui::Begin("Profiler");
    ImGui::Text("CPU");
    ImGui::Text("  frame  %6.2f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Separator();
    ImGui::Text("GPU");
    float gpuTotal{0.0f};
    for (const auto& r : gpuTimer.getLastResults()) {
      ImGui::Text("  %-8s %6.3f ms", r.name.c_str(), r.ms);
      gpuTotal += r.ms;
    }
    ImGui::Text("  %-8s %6.3f ms", "total", gpuTotal);
    ImGui::End();

    // Record
    VkCommandBuffer cb = cmdManager.get(frameInFlightIndex);
    vkCheck(vkResetCommandBuffer(cb, 0));
    VkCommandBufferBeginInfo cbBI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkCheck(vkBeginCommandBuffer(cb, &cbBI));

    // Readback previous results for this slot + reset the pool range. Must
    // happen before any vkCmdBeginRendering.
    gpuTimer.beginFrame(cb, frameInFlightIndex);

    // Frame graph: declare resources + passes, let it insert barriers
    // and drive dynamic rendering. Swapchain image and depth are imported
    // (owned elsewhere), so no per-frame allocation happens here.
    FrameGraph fg(ctx);
    FgResource backbuffer = fg.importImage(
      "backbuffer", swapchain.images[swapchainImageIndex],
      swapchain.views[swapchainImageIndex], swapchain.format, swapchain.extent);
    FgResource depth = fg.importImage(
      "depth", depthImages[frameInFlightIndex].handle,
      depthImages[frameInFlightIndex].view, depthFormat, swapchain.extent);

    fg.addPass(
      "forward",
      {{backbuffer, FgUsage::ColorAttachment},
       {depth,
        FgUsage::DepthAttachment,
        {.clearValue{.depthStencil{1.0f, 0}}}}},
      [&](VkCommandBuffer cb) {
        VkViewport vp{.width = static_cast<float>(windowSize.x),
                      .height = static_cast<float>(windowSize.y),
                      .minDepth = 0.0f,
                      .maxDepth = 1.0f};
        vkCmdSetViewport(cb, 0, 1, &vp);
        VkRect2D scissor{.extent = swapchain.extent};
        vkCmdSetScissor(cb, 0, 1, &scissor);

        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1, &descriptorSet, 0,
                                nullptr);
        VkDeviceSize vOffset{0};
        vkCmdBindVertexBuffers(cb, 0, 1, &geometryBuffer.handle, &vOffset);
        vkCmdBindIndexBuffer(cb, geometryBuffer.handle, vBufSize,
                             VK_INDEX_TYPE_UINT16);
        vkCmdPushConstants(
          cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
          sizeof(VkDeviceAddress),
          &shaderDataBuffers[frameInFlightIndex].deviceAddress);
        vkCmdDrawIndexed(cb, static_cast<uint32_t>(indices.size()), 3, 0, 0, 0);
      });

    fg.addPass("imgui",
               {{backbuffer,
                 FgUsage::ColorAttachment,
                 {.load = VK_ATTACHMENT_LOAD_OP_LOAD}}},
               [&](VkCommandBuffer cb) {
                 // ImGui draws into the same backbuffer scope, on top of the scene.
                 ui.draw(cb);
               });

    fg.addPass("present", {{backbuffer, FgUsage::Present}}, {});

    fg.execute(cb, &gpuTimer);
    vkCheck(vkEndCommandBuffer(cb));

    // Submit
    VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &sync.presentSemaphores[frameInFlightIndex],
      .pWaitDstStageMask = &waitStage,
      .commandBufferCount = 1,
      .pCommandBuffers = &cb,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &sync.renderSemaphores[swapchainImageIndex]};
    vkCheck(vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo,
                          sync.frameFences[frameInFlightIndex]));
    gpuTimer.endFrame();
    frameInFlightIndex = (frameInFlightIndex + 1) % kFramesInFlight;

    // Present
    VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &sync.renderSemaphores[swapchainImageIndex],
      .swapchainCount = 1,
      .pSwapchains = &swapchain.handle,
      .pImageIndices = &swapchainImageIndex};
    VkResult presentResult = vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
      updateSwapchain = true;
    else
      vkCheck(presentResult);

    // Events
    float elapsedTime = (SDL_GetTicks() - lastTime) / 1000.0f;
    lastTime = SDL_GetTicks();
    for (SDL_Event event; SDL_PollEvent(&event);) {
      ui.processEvent(event);
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      }
      if (ui.wantCaptureMouse() && (event.type == SDL_EVENT_MOUSE_MOTION ||
                                    event.type == SDL_EVENT_MOUSE_WHEEL ||
                                    event.type == SDL_EVENT_MOUSE_BUTTON_DOWN))
        continue;  // ImGui is using the cursor; don't move the camera
      if (event.type == SDL_EVENT_MOUSE_MOTION &&
          event.button.button == SDL_BUTTON_LEFT) {
        objectRotations[shaderData.selected].x -=
          (float)event.motion.yrel * elapsedTime;
        objectRotations[shaderData.selected].y +=
          (float)event.motion.xrel * elapsedTime;
      }
      if (event.type == SDL_EVENT_MOUSE_WHEEL)
        camPos.z += (float)event.wheel.y * elapsedTime * 10.0f;
      if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_PLUS || event.key.key == SDLK_KP_PLUS)
          shaderData.selected =
            (shaderData.selected < 2) ? shaderData.selected + 1 : 0;
        if (event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS)
          shaderData.selected =
            (shaderData.selected > 0) ? shaderData.selected - 1 : 2;
      }
      if (event.type == SDL_EVENT_WINDOW_RESIZED)
        updateSwapchain = true;
    }

    // Swapchain recreate
    if (updateSwapchain) {
      sdlCheck(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
      updateSwapchain = false;
      vkCheck(vkDeviceWaitIdle(ctx.device));
      swapchain.recreate(ctx, surface, windowSize);
      for (int i = 0; i < depthImages.size(); i++) {
        depthImages[i].destroy(ctx.device, ctx.allocator);
        depthImages[i] =
          Image::create2D(ctx.allocator, ctx.device, depthFormat,
                          {static_cast<uint32_t>(windowSize.x),
                           static_cast<uint32_t>(windowSize.y)},
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                          VK_IMAGE_ASPECT_DEPTH_BIT);
      }
    }
  }

  // --- Teardown ---
  vkCheck(vkDeviceWaitIdle(ctx.device));

  ui.destroy(ctx);
  gpuTimer.destroy();

  for (auto& buf : shaderDataBuffers)
    buf.destroy(ctx.allocator);
  geometryBuffer.destroy(ctx.allocator);
  for (auto& tex : textures)
    tex.destroy(ctx.device, ctx.allocator);
  for (int i = 0; i < depthImages.size(); i++) {
    depthImages[i].destroy(ctx.device, ctx.allocator);
  }
  swapchain.destroy(ctx.device);
  vkDestroyDescriptorSetLayout(ctx.device, descriptorSetLayout, nullptr);
  vkDestroyDescriptorPool(ctx.device, descriptorPool, nullptr);
  vkDestroyPipelineLayout(ctx.device, pipelineLayout, nullptr);
  vkDestroyPipeline(ctx.device, pipeline, nullptr);
  shaderModule.destroy(ctx.device);
  sync.destroy(ctx.device);
  cmdManager.destroy(ctx.device);
  vkDestroySurfaceKHR(ctx.instance, surface, nullptr);
  ctx.destroy();

  SDL_DestroyWindow(window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  SDL_Quit();

  return 0;
}
