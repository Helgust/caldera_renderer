#pragma once
#include <volk/volk.h>
#include <vector>

namespace caldera {

// Fluent builder — set what you need, rest uses sane defaults.
// Designed so deferred/forward passes just call it twice with
// different attachment formats and vertex input state.
struct GraphicsPipelineBuilder {
  GraphicsPipelineBuilder& setShaderStages(VkShaderModule module,
                                           const char* vertEntry = "main",
                                           const char* fragEntry = "main");

  GraphicsPipelineBuilder& setVertexInput(
    const VkVertexInputBindingDescription& binding,
    const std::vector<VkVertexInputAttributeDescription>& attrs);

  GraphicsPipelineBuilder& setDepthStencil(
    bool testEnable, bool writeEnable,
    VkCompareOp compareOp = VK_COMPARE_OP_LESS_OR_EQUAL);

  GraphicsPipelineBuilder& setColorAttachmentFormat(VkFormat format);
  GraphicsPipelineBuilder& setDepthAttachmentFormat(VkFormat format);
  GraphicsPipelineBuilder& setPipelineLayout(VkPipelineLayout layout);

  // Defaults: TRIANGLE_LIST, dynamic viewport+scissor, no MSAA, no cull
  VkPipeline build(VkDevice device);

 private:
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages_;
  VkVertexInputBindingDescription vertexBinding_{};
  std::vector<VkVertexInputAttributeDescription> vertexAttribs_;
  bool hasVertexInput_{false};
  bool depthTestEnable_{false};
  bool depthWriteEnable_{false};
  VkCompareOp depthCompareOp_{VK_COMPARE_OP_LESS_OR_EQUAL};
  VkFormat colorFormat_{VK_FORMAT_UNDEFINED};
  VkFormat depthFormat_{VK_FORMAT_UNDEFINED};
  VkPipelineLayout layout_{VK_NULL_HANDLE};
};

}  // namespace caldera
