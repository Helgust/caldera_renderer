# TODO

Active near-term backlog. Full per-step detail is in the Notion board
*Caldera Renderer — Development Roadmap*; long-term phases are in `ROADMAP.md`.

## Now — Stage 1: Vulkan Foundation

- [ ] **1.1 Validation layers** — `VK_LAYER_KHRONOS_validation` + debug messenger in `vulkanContext.cpp`, gated on `!NDEBUG`. *(do this first — catches every later bug)*
- [ ] **1.2 Debug labels / object names** — `vkCmdBeginDebugUtilsLabelEXT` around passes, name buffers/images
- [ ] **1.3 GPU timestamp queries** — `VkQueryPool` in new `core/gpuTimer.h`, per-pass timing
- [ ] **1.4 Wire up ImGui** — init + second rendering pass for UI, stats overlay
- [ ] **1.5 Synchronization audit** — comment every `vkCmdPipelineBarrier2`, tighten stage masks

## Next — Stage 2: Renderer Architecture

- [ ] **2.1 Deletion queue** — `core/deletionQueue.h`, replace manual cleanup in `main.cpp`
- [ ] **2.2 Descriptor allocator** — pool-of-pools + `DescriptorLayoutBuilder`
- [ ] **2.3 Material system** — `MaterialTemplate` / `Material`, move Phong pipeline out of `main.cpp`
- [ ] **2.4 Shader reflection** *(optional)* — drive layouts from Slang reflection
- [ ] **2.5a Render graph: data structures** — passes, resource handles, declarations
- [ ] **2.5b Render graph: dependency tracking** — adjacency list + topological sort
- [ ] **2.5c Render graph: barrier generation** — auto `VkImageMemoryBarrier2`, remove manual barriers
- [ ] **2.5d Render graph: transient resources** — lifetime tracking + memory aliasing

## Later — Stage 3: Modern Rendering

- [ ] **3.1 PBR material** — Cook-Torrance BRDF, metallic/roughness from glTF
- [ ] **3.2 IBL** — irradiance / prefilter / BRDF LUT (first compute shaders)
- [ ] **3.3 Shadow mapping** — depth-only shadow pass via render graph
- [ ] **3.4 GPU culling / indirect drawing** — compute frustum cull → `vkCmdDrawIndexedIndirectCount`

## Backlog — Stage 4: Engine Grade

- [ ] **4.1 Full bindless resource table**
- [ ] **4.2 Async compute** (separate queue)
- [ ] **4.3 Hot-reload shaders**
- [ ] **4.4 Mesh shaders**
- [ ] **4.5 Ray tracing** (shadows + reflections)

## Misc

- [ ] Setup clang-tidy
