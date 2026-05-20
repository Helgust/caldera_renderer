# TODO

Active near-term backlog. Full per-step detail is in the Notion board
*Caldera Renderer — Development Roadmap*; long-term phases are in `ROADMAP.md`.

## Done — Stage 1: Vulkan Foundation

- [x] **1.1 Validation layers** — `VK_LAYER_KHRONOS_validation` + debug messenger in `vulkanContext.cpp`, gated on `!NDEBUG`
- [x] **1.2 Debug labels / object names** — `core/debugUtils.h`, scoped labels around framegraph passes, named buffers/images
- [x] **1.3 GPU timestamp queries** — `core/gpuTimer.h`, per-pass timing surfaced in the Profiler window
- [x] **1.4 ImGui overlay** — wired through `ui/imguiLayer`, live FPS / GPU timings / scene controls

## Now — Stage 1 wrap-up

- [ ] **1.5 Synchronization audit** — re-scoped: audit `FrameGraph` barrier derivation (`FgResourceState` transitions), confirm stage masks are tight, document the present submit's `waitDstStageMask`
- [ ] **1.6 Camera system** — extract from `main.cpp` (currently fixed perspective + Z translate); add free-fly or arcball, separate view/proj from `ShaderData` upload path

## Next — Stage 2: Renderer Architecture

- [ ] **2.1 Deletion queue** — `core/deletionQueue.h`, replace manual cleanup in `main.cpp`
- [ ] **2.2 Descriptor allocator** — pool-of-pools + `DescriptorLayoutBuilder` (current code uses a single hardcoded pool + inline layout)
- [ ] **2.3 Material system** — `MaterialTemplate` / `Material`, move Phong pipeline out of `main.cpp`
- [ ] **2.4 Shader reflection** *(optional)* — drive layouts from Slang reflection
- [x] **2.5a Render graph: data structures** — passes, resource handles, per-attachment load/store ops
- [ ] **2.5b Render graph: real dependency tracking** — replace declaration-order execution with adjacency list + topological sort (prerequisite for reordering / async compute)
- [x] **2.5c Render graph: barrier generation** — auto `VkImageMemoryBarrier2` from `FgResourceState`
- [ ] **2.5d Render graph: transient resources** — lifetime tracking + memory aliasing across non-overlapping passes

## Later — Stage 3: Modern Rendering

- [ ] **3.1 PBR material** — Cook-Torrance BRDF, metallic/roughness from glTF
- [ ] **3.2 IBL** — irradiance / prefilter / BRDF LUT (first compute shaders)
- [ ] **3.3 Shadow mapping** — depth-only shadow pass via render graph
- [ ] **3.4 GPU culling / indirect drawing** — compute frustum cull → `vkCmdDrawIndexedIndirectCount`

## Backlog — Stage 4: Engine Grade

- [ ] **4.1 Full bindless resource table**
- [ ] **4.2 Async compute** (separate queue — depends on 2.5b/2.5d)
- [ ] **4.3 Hot-reload shaders**
- [ ] **4.4 Mesh shaders**
- [ ] **4.5 Ray tracing** (shadows + reflections)

## Misc

- [ ] Setup clang-tidy
