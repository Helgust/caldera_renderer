#pragma once
#include <volk/volk.h>
#include <string>
#include "slang/slang-com-ptr.h"
#include "slang/slang.h"

namespace caldera {

// Owns the global Slang session — one per app lifetime, init once in main
struct SlangCompiler {
  Slang::ComPtr<slang::IGlobalSession> global;
  Slang::ComPtr<slang::ISession> session;

  void init();
};

// Owns a single VkShaderModule compiled from a Slang source file
struct ShaderModule {
  VkShaderModule handle{VK_NULL_HANDLE};

  static ShaderModule loadSlang(VkDevice device, slang::ISession* session,
                                const std::string& path,
                                const std::string& moduleName);

  void destroy(VkDevice device);
};

}  // namespace caldera
