#include "core/vulkanContext.h"
#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <optional>
#include <vector>
#include "core/assert.h"
#include "core/debugUtils.h"

namespace caldera {

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
              VkDebugUtilsMessageTypeFlagsEXT,
              const VkDebugUtilsMessengerCallbackDataEXT* data, void*) {
  // Route through the shared sink (debugger Output window + caldera.log, not
  // just the console) at the matching severity. pMessage is passed as an
  // argument, never as the format string, so stray '%' in it stay literal.
  if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    logError("[VK] %s", data->pMessage);
  else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    logWarn("[VK] %s", data->pMessage);
  else
    logInfo("[VK] %s", data->pMessage);
  // Break on validation ERRORs so the offending call is on top of the stack
  // when a debugger is attached; with none, fall through (already logged).
  if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    breakIfDebugger();
  return VK_FALSE;
}

static bool hasLayer(const char* name) {
  uint32_t count{0};
  if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS)
    return false;
  std::vector<VkLayerProperties> props(count);
  if (vkEnumerateInstanceLayerProperties(&count, props.data()) != VK_SUCCESS)
    return false;
  return std::any_of(props.begin(), props.end(),
                     [&](const VkLayerProperties& p) {
                       return std::strcmp(p.layerName, name) == 0;
                     });
}

// Queue family
static std::optional<uint32_t> findGraphicsFamily(VkPhysicalDevice candidate) {
  uint32_t count{0};
  vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, nullptr);
  std::vector<VkQueueFamilyProperties> families(count);
  vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, families.data());

  for (uint32_t i = 0; i < families.size(); i++) {
    if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      return i;
    }
  }
  return {};
}

void VulkanContext::init() {
  // Taken directly from your main() — Instance block
  volkInitialize();

  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = "CalderaRenderer",
                            .apiVersion = VK_API_VERSION_1_3};

  uint32_t extCount{0};
  const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&extCount);

#ifdef NDEBUG
  constexpr bool wantValidation = false;
#else
  constexpr bool wantValidation = true;
#endif

  const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";
  const bool enableValidation = wantValidation && hasLayer(kValidationLayer);
  if (wantValidation && !enableValidation) {
    std::cerr << "[VK WARN ] " << kValidationLayer
              << " not available — continuing without validation. "
                 "Install the Vulkan SDK or set VK_LAYER_PATH.\n";
  }

  std::vector<const char*> exts(sdlExts, sdlExts + extCount);
  if (enableValidation)
    exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  const char* layers[] = {kValidationLayer};

  VkDebugUtilsMessengerCreateInfoEXT messengerCI{
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debugCallback};

  VkInstanceCreateInfo instanceCI{
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = enableValidation ? &messengerCI : nullptr,
    .pApplicationInfo = &appInfo,
    .enabledLayerCount = enableValidation ? 1u : 0u,
    .ppEnabledLayerNames = enableValidation ? layers : nullptr,
    .enabledExtensionCount = static_cast<uint32_t>(exts.size()),
    .ppEnabledExtensionNames = exts.data()};
  vkCheck(vkCreateInstance(&instanceCI, nullptr, &instance));
  volkLoadInstance(instance);

  // Arm the debug-utils helpers exactly when the extension is live (it is
  // added above only alongside the validation layer). Single gate for all
  // setObjectName / debug-label calls across the codebase.
  gDebugUtilsEnabled = enableValidation;

  if (enableValidation) {
    vkCheck(vkCreateDebugUtilsMessengerEXT(instance, &messengerCI, nullptr,
                                           &debugMessenger));
  }

  // Physical device — from your Device block
  uint32_t devCount{0};
  vkCheck(vkEnumeratePhysicalDevices(instance, &devCount, nullptr));
  std::vector<VkPhysicalDevice> devices(devCount);
  vkCheck(vkEnumeratePhysicalDevices(instance, &devCount, devices.data()));
  VkPhysicalDevice discreteGpu = VK_NULL_HANDLE;
  VkPhysicalDevice integratedGpu = VK_NULL_HANDLE;
  for (VkPhysicalDevice candidate : devices) {
    VkPhysicalDeviceProperties2 props{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    vkGetPhysicalDeviceProperties2(candidate, &props);

    // Only consider devices we can actually drive: a graphics queue (and
    // present — see question below).
    if (!findGraphicsFamily(candidate)) {
      continue;
    }

    switch (props.properties.deviceType) {
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        discreteGpu = candidate;
        break;  // best case — stop looking
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        integratedGpu = candidate;  // remember, keep looking for a discrete
        break;
      default:
        break;
    }

    if (discreteGpu != VK_NULL_HANDLE) {
      break;
    }
  }

  physicalDevice =
    (discreteGpu != VK_NULL_HANDLE) ? discreteGpu : integratedGpu;
  CALDERA_ASSERT_MSG(physicalDevice != VK_NULL_HANDLE, "No suitable GPU found");
  graphicsFamily = findGraphicsFamily(physicalDevice).value();

  VkPhysicalDeviceProperties2 props{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  vkGetPhysicalDeviceProperties2(physicalDevice, &props);
  std::cout << "Selected device: " << props.properties.deviceName << "\n";

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
