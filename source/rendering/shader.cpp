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

  // Capture diagnostics from both stages: without the out-param a typo in the
  // .slang source returned a null module and this function dereferenced it
  // with no message. Slang writes the human-readable error into the blob.
  Slang::ComPtr<ISlangBlob> loadDiag;
  Slang::ComPtr<slang::IModule> slangModule{session->loadModuleFromSource(
    module_name.c_str(), path.c_str(), nullptr, loadDiag.writeRef())};
  CALDERA_ASSERT_MSG(
    slangModule, "slang: failed to load '%s': %s", path.c_str(),
    loadDiag ? (const char*)loadDiag->getBufferPointer() : "(no diagnostics)");

  Slang::ComPtr<ISlangBlob> spirv;
  Slang::ComPtr<ISlangBlob> codeDiag;
  const SlangResult codeResult =
    slangModule->getTargetCode(0, spirv.writeRef(), codeDiag.writeRef());
  CALDERA_ASSERT_MSG(
    SLANG_SUCCEEDED(codeResult) && spirv, "slang: codegen failed for '%s': %s",
    path.c_str(),
    codeDiag ? (const char*)codeDiag->getBufferPointer() : "(no diagnostics)");

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
