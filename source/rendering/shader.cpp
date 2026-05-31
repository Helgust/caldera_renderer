#include "rendering/shader.h"
#include <array>
#include "core/vulkanContext.h"

namespace caldera {

void SlangCompiler::init() {
  slang::createGlobalSession(global.writeRef());

  auto targets = std::to_array<slang::TargetDesc>(
    {{.format{SLANG_SPIRV}, .profile{global->findProfile("spirv_1_4")}}});

  auto options = std::to_array<slang::CompilerOptionEntry>(
    {{slang::CompilerOptionName::EmitSpirvDirectly,
      {slang::CompilerOptionValueKind::Int, 1}}});

  slang::SessionDesc sessionDesc{
    .targets{targets.data()},
    .targetCount{SlangInt(targets.size())},
    .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
    .compilerOptionEntries{options.data()},
    .compilerOptionEntryCount{uint32_t(options.size())}};

  global->createSession(sessionDesc, session.writeRef());
}

ShaderModule ShaderModule::loadSlang(VkDevice device, slang::ISession* session,
                                     const std::string& path,
                                     const std::string& module_name) {
  ShaderModule sm{};

  Slang::ComPtr<slang::IModule> slangModule{session->loadModuleFromSource(
    module_name.c_str(), path.c_str(), nullptr, nullptr)};

  Slang::ComPtr<ISlangBlob> spirv;
  slangModule->getTargetCode(0, spirv.writeRef());

  VkShaderModuleCreateInfo shaderCI{
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = spirv->getBufferSize(),
    .pCode = static_cast<const uint32_t*>(spirv->getBufferPointer())};

  vkCheck(vkCreateShaderModule(device, &shaderCI, nullptr, &sm.handle));
  return sm;
}

void ShaderModule::destroy(VkDevice device) {
  vkDestroyShaderModule(device, handle, nullptr);
  handle = VK_NULL_HANDLE;
}

}  // namespace caldera
