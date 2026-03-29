#include "core/vulkanContext.h"
#include <SDL3/SDL_vulkan.h>
#include <cassert>
#include <iostream>
#include <vector>

namespace caldera {

void VulkanContext::init(uint32_t deviceIndex) {
  // Taken directly from your main() — Instance block
  volkInitialize();

  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = "CalderaRenderer",
                            .apiVersion = VK_API_VERSION_1_3};

  uint32_t extCount{0};
  const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&extCount);

  VkInstanceCreateInfo instanceCI{
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &appInfo,
    .enabledExtensionCount = extCount,
    .ppEnabledExtensionNames = exts};
  vkCheck(vkCreateInstance(&instanceCI, nullptr, &instance));
  volkLoadInstance(instance);

  // Physical device — from your Device block
  uint32_t devCount{0};
  vkCheck(vkEnumeratePhysicalDevices(instance, &devCount, nullptr));
  std::vector<VkPhysicalDevice> devices(devCount);
  vkCheck(vkEnumeratePhysicalDevices(instance, &devCount, devices.data()));
  assert(deviceIndex < devCount);
  physicalDevice = devices[deviceIndex];

  VkPhysicalDeviceProperties2 props{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  vkGetPhysicalDeviceProperties2(physicalDevice, &props);
  std::cout << "Selected device: " << props.properties.deviceName << "\n";

  // Queue family
  uint32_t qfCount{0};
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfCount, nullptr);
  std::vector<VkQueueFamilyProperties> qfs(qfCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfCount,
                                           qfs.data());
  for (uint32_t i = 0; i < qfCount; i++) {
    if (qfs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphicsFamily = i;
      break;
    }
  }

  // Logical device — your full feature chain
  const float priority{1.0f};
  VkDeviceQueueCreateInfo queueCI{
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = graphicsFamily,
    .queueCount = 1,
    .pQueuePriorities = &priority};

  VkPhysicalDeviceVulkan12Features vk12{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .descriptorIndexing = true,
    .shaderSampledImageArrayNonUniformIndexing = true,
    .descriptorBindingVariableDescriptorCount = true,
    .runtimeDescriptorArray = true,
    .bufferDeviceAddress = true};

  VkPhysicalDeviceVulkan13Features vk13{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .pNext = &vk12,
    .synchronization2 = true,
    .dynamicRendering = true};

  VkPhysicalDeviceFeatures vk10{.samplerAnisotropy = VK_TRUE};

  const char* devExts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo devCI{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                           .pNext = &vk13,
                           .queueCreateInfoCount = 1,
                           .pQueueCreateInfos = &queueCI,
                           .enabledExtensionCount = 1,
                           .ppEnabledExtensionNames = devExts,
                           .pEnabledFeatures = &vk10};
  vkCheck(vkCreateDevice(physicalDevice, &devCI, nullptr, &device));
  vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);

  // VMA
  VmaVulkanFunctions vmaFns{.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
                            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
                            .vkCreateImage = vkCreateImage};
  VmaAllocatorCreateInfo allocCI{
    .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    .physicalDevice = physicalDevice,
    .device = device,
    .pVulkanFunctions = &vmaFns,
    .instance = instance};
  vkCheck(vmaCreateAllocator(&allocCI, &allocator));
}

void VulkanContext::destroy() {
  vmaDestroyAllocator(allocator);
  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(instance, nullptr);
}

}  // namespace caldera
