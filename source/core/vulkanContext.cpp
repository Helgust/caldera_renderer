#include "core/vulkanContext.h"
#include <SDL3/SDL_vulkan.h>
#include <cassert>
#include <iostream>
#include <vector>
#include "core/validation.h"

namespace caldera {

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
              VkDebugUtilsMessageTypeFlagsEXT,
              const VkDebugUtilsMessengerCallbackDataEXT* data, void*) {
  const char* prefix =
    (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)     ? "ERROR"
    : (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "WARN "
                                                                   : "INFO ";
  std::cout << "[VK " << prefix << "] " << data->pMessage << "\n";
  return VK_FALSE;
}

void VulkanContext::init(uint32_t deviceIndex) {
  // Taken directly from your main() — Instance block
  volkInitialize();

  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = "CalderaRenderer",
                            .apiVersion = VK_API_VERSION_1_3};

  uint32_t extCount{0};
  const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&extCount);

#ifdef NDEBUG
  constexpr bool enableValidation = false;
#else
  constexpr bool enableValidation = true;
#endif

  std::vector<const char*> exts(sdlExts, sdlExts + extCount);
  if (enableValidation)
    exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  // Create an object with all the settigns initialized
  LayerSettings layerSettings;

  // We can modify the setting values:
  layerSettings.validation.report_flags = {"error", "warn"};

  std::vector<VkLayerSettingEXT> layerSettingInfo = layerSettings.info();

  const VkLayerSettingsCreateInfoEXT layerSettingsCI = {
    VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr,
    static_cast<uint32_t>(layerSettingInfo.size()), layerSettingInfo.data()};
  const char* layers[] = {"VK_LAYER_KHRONOS_validation"};

  VkInstanceCreateInfo instanceCI{
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = enableValidation ? &layerSettingsCI : nullptr,
    .pApplicationInfo = &appInfo,
    .enabledLayerCount = enableValidation ? 1 : 0,
    .ppEnabledLayerNames = enableValidation ? layers : nullptr,
    .enabledExtensionCount = static_cast<uint32_t>(exts.size()),
    .ppEnabledExtensionNames = exts.data()};
  vkCheck(vkCreateInstance(&instanceCI, nullptr, &instance));
  volkLoadInstance(instance);

  if (enableValidation) {
    VkDebugUtilsMessengerCreateInfoEXT messengerCI{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugCallback};
    vkCheck(vkCreateDebugUtilsMessengerEXT(instance, &messengerCI, nullptr,
                                           &debugMessenger));
  }

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
  if (debugMessenger != VK_NULL_HANDLE)
    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  vkDestroyInstance(instance, nullptr);
}

}  // namespace caldera
