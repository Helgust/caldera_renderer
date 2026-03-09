#define VOLK_IMPLEMENTATION
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk/volk.h>
#include <vulkan/vulkan.h>
#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <ktx.h>
#include <ktxvulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "slang/slang-com-ptr.h"
#include "slang/slang.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

VkInstance instance{VK_NULL_HANDLE};
VkDevice device{VK_NULL_HANDLE};
VkQueue queue{VK_NULL_HANDLE};
VmaAllocator allocator{VK_NULL_HANDLE};
VkSurfaceKHR surface{VK_NULL_HANDLE};
VkSwapchainKHR swapchain{VK_NULL_HANDLE};
std::vector<VkImage> swapchainImages;
std::vector<VkImageView> swapchainImageViews;

static inline void chk(VkResult result) {
  if (result != VK_SUCCESS) {
    std::cerr << "Vulkan call returned an error (" << result << ")\n";
    exit(result);
  }
}

static inline void chk(bool result) {
  if (!result) {
    std::cerr << "Call returned an error\n";
    exit(result);
  }
}

int main(int argc, char* argv[]) {
  chk(SDL_Init(SDL_INIT_VIDEO));
  chk(SDL_Vulkan_LoadLibrary(NULL));
  volkInitialize();
  // Instance
  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = "CalderaRenderer",
                            .apiVersion = VK_API_VERSION_1_3};
  uint32_t instanceExtensionsCount{0};
  char const* const* instanceExtensions{
      SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount)};
  VkInstanceCreateInfo instanceCI{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = instanceExtensionsCount,
      .ppEnabledExtensionNames = instanceExtensions,
  };
  chk(vkCreateInstance(&instanceCI, nullptr, &instance));
  volkLoadInstance(instance);

  // Device
  uint32_t deviceCount{0};
  chk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
  std::vector<VkPhysicalDevice> devices(deviceCount);
  chk(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));
  uint32_t deviceIndex{0};
  if (argc > 1) {
    deviceIndex = std::stoi(argv[1]);
    assert(deviceIndex < deviceCount);
  }
  VkPhysicalDeviceProperties2 deviceProperties{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
  std::cout << "Selected device: " << deviceProperties.properties.deviceName
            << "\n";

  uint32_t queueFamilyCount{0};
  vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex],
                                           &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
      devices[deviceIndex], &queueFamilyCount, queueFamilies.data());
  // Find a queue family for graphics
  uint32_t queueFamily{0};
  for (size_t i = 0; i < queueFamilies.size(); i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queueFamily = i;
      break;
    }
  }
  chk(SDL_Vulkan_GetPresentationSupport(instance, devices[deviceIndex],
                                        queueFamily));
  // Logical device
  const float qfpriorities{1.0f};
  VkDeviceQueueCreateInfo queueCI{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queueFamily,
      .queueCount = 1,
      .pQueuePriorities = &qfpriorities};
  VkPhysicalDeviceVulkan12Features enabledVk12Features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .descriptorIndexing = true,
      .shaderSampledImageArrayNonUniformIndexing = true,
      .descriptorBindingVariableDescriptorCount = true,
      .runtimeDescriptorArray = true,
      .bufferDeviceAddress = true};
  const VkPhysicalDeviceVulkan13Features enabledVk13Features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &enabledVk12Features,
      .synchronization2 = true,
      .dynamicRendering = true,
  };
  const std::vector<const char*> deviceExtensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  const VkPhysicalDeviceFeatures enabledVk10Features{.samplerAnisotropy =
                                                         VK_TRUE};

  VkDeviceCreateInfo deviceCI{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &enabledVk13Features,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueCI,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = &enabledVk10Features};
  chk(vkCreateDevice(devices[deviceIndex], &deviceCI, nullptr, &device));
  vkGetDeviceQueue(device, queueFamily, 0, &queue);

  // VMA
  VmaVulkanFunctions vkFunctions{.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
                                 .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
                                 .vkCreateImage = vkCreateImage};
  VmaAllocatorCreateInfo allocatorCI{
      .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = devices[deviceIndex],
      .device = device,
      .pVulkanFunctions = &vkFunctions,
      .instance = instance};
  chk(vmaCreateAllocator(&allocatorCI, &allocator));

  //Window
  SDL_Window* window =
      SDL_CreateWindow("Caldera Renderer", 1280u, 720u,
                       SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  assert(window);
  chk(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));
  VkSurfaceCapabilitiesKHR surfaceCaps{};
  chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], surface,
                                                &surfaceCaps));

  //Swapchain
  const VkFormat imageFormat{VK_FORMAT_B8G8R8A8_SRGB};
  VkSwapchainCreateInfoKHR swapchainCI{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = surfaceCaps.minImageCount,
      .imageFormat = imageFormat,
      .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
      .imageExtent{.width = surfaceCaps.currentExtent.width,
                   .height = surfaceCaps.currentExtent.height},
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR};
  chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));
  uint32_t imageCount{0};
  chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
  swapchainImages.resize(imageCount);
  chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount,
                              swapchainImages.data()));
  swapchainImageViews.resize(imageCount);

  for (auto i = 0; i < imageCount; i++) {
    VkImageViewCreateInfo viewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapchainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageFormat,
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                          .levelCount = 1,
                          .layerCount = 1}};
    chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
  }

  return 0;
}
