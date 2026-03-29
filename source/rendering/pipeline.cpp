#include "rendering/pipeline.h"
#include "core/vulkanContext.h"

namespace caldera {

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setShaderStages(
  VkShaderModule module, const char* vertEntry, const char* fragEntry) {
  shaderStages_ = {
    {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
     .stage = VK_SHADER_STAGE_VERTEX_BIT,
     .module = module,
     .pName = vertEntry},
    {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
     .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
     .module = module,
     .pName = fragEntry}};
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexInput(
  const VkVertexInputBindingDescription& binding,
  const std::vector<VkVertexInputAttributeDescription>& attrs) {
  vertexBinding_ = binding;
  vertexAttribs_ = attrs;
  hasVertexInput_ = true;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepthStencil(
  bool testEnable, bool writeEnable, VkCompareOp compareOp) {
  depthTestEnable_ = testEnable;
  depthWriteEnable_ = writeEnable;
  depthCompareOp_ = compareOp;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setColorAttachmentFormat(
  VkFormat format) {
  colorFormat_ = format;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepthAttachmentFormat(
  VkFormat format) {
  depthFormat_ = format;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setPipelineLayout(
  VkPipelineLayout layout) {
  layout_ = layout;
  return *this;
}

VkPipeline GraphicsPipelineBuilder::build(VkDevice device) {
  VkPipelineVertexInputStateCreateInfo vertexInputState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  if (hasVertexInput_) {
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.pVertexBindingDescriptions = &vertexBinding_;
    vertexInputState.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(vertexAttribs_.size());
    vertexInputState.pVertexAttributeDescriptions = vertexAttribs_.data();
  }

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

  std::vector<VkDynamicState> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT,
                                            VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates = dynamicStates.data()};

  VkPipelineViewportStateCreateInfo viewportState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1};

  VkPipelineRasterizationStateCreateInfo rasterState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .lineWidth = 1.0f};

  VkPipelineMultisampleStateCreateInfo msaaState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

  VkPipelineDepthStencilStateCreateInfo depthState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = depthTestEnable_ ? VK_TRUE : VK_FALSE,
    .depthWriteEnable = depthWriteEnable_ ? VK_TRUE : VK_FALSE,
    .depthCompareOp = depthCompareOp_};

  VkPipelineColorBlendAttachmentState blendAttachment{.colorWriteMask = 0xF};
  VkPipelineColorBlendStateCreateInfo colorBlendState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &blendAttachment};

  VkPipelineRenderingCreateInfo renderingCI{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &colorFormat_,
    .depthAttachmentFormat = depthFormat_};

  VkGraphicsPipelineCreateInfo pipelineCI{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &renderingCI,
    .stageCount = static_cast<uint32_t>(shaderStages_.size()),
    .pStages = shaderStages_.data(),
    .pVertexInputState = &vertexInputState,
    .pInputAssemblyState = &inputAssembly,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterState,
    .pMultisampleState = &msaaState,
    .pDepthStencilState = &depthState,
    .pColorBlendState = &colorBlendState,
    .pDynamicState = &dynamicState,
    .layout = layout_};

  VkPipeline pipeline{VK_NULL_HANDLE};
  vkCheck(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI,
                                    nullptr, &pipeline));
  return pipeline;
}

}  // namespace caldera
