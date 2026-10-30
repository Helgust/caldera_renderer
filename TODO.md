# TODO

Active near-term backlog and the **source of truth for task status**. Per-step notes live in
Obsidian (no status there); long-term phases are in `ROADMAP.md`, which is not used for planning in 2027.

## Caldera 2027 — scope

**Final demo (Dec 2027):** Sponza (Khronos glTF Sample Models) with hundreds of point lights, a
free camera, an ImGui toggle per technique, a per-pass ms table from GPU timestamps / Tracy, and a
README with a frame breakdown.

| Milestone | Target | Items |
|---|---|---|
| **A** | Nov 2026 – Feb 7, 2027 | 1.6 Camera, 1.5 Sync audit, 1.7 Tracy, 2.7 Multi-mesh scene loading, 3.1 PBR, 3.5 HDR + tonemapping |
| **B** | Feb 8 – Mar 14 | 2.5b Topological sort, 2.5d Transient resources, 3.3 Shadow mapping |
| **C** | Mar 15 – May 30 | C1: 3.4 GPU culling + stream compaction. C2: 3.6 Deferred, 3.7 Clustered lighting + ms table |
| **D** | Jun – Aug | One of IBL (2.6a–c + 3.2), CSM or SSAO — chosen at the end of May |
| **E** | Sep – Oct | 3.8 TAA |
| **F** | Nov – Dec, stretch | 4.6 LBVH + compute shadows, hardware ray query as the reference |

**Not in 2027:** 1.8 Threading, 2.4 Shader reflection, 4.1–4.4 (full bindless, async compute, hot
reload, mesh shaders), RT reflections, virtual shadow maps, scene graph, a full material system,
side-topic mini-programs. Unlisted items (2.1, 2.2, 2.3, R.1, R.2) are done only when a scoped item
needs them.

**How a milestone runs:**

1. Topic → reading (books in `../Graphics`, articles, docs) → quiz.
2. Design page → AI review.
3. Work-style tickets from the AI (task, acceptance criteria, constraints). Core code is written by
   hand and reviewed by the AI; scaffolding may be generated. Same checklist as at work: a first
   patchset I would merge, numbers in the commit message.
4. Last ticket is a bug: the AI plants one in my code, I find it with RenderDoc and validation layers.

**Done means:** builds with one command, screenshots or GIF, README with decisions, trade-offs and
numbers, one GPU-timestamp number, and a line on what was written by hand vs generated.

**Planning:** once per cycle, 30 minutes on its first evening. Every other evening starts at the
first unchecked item of the current milestone.

## Done — Stage 1: Vulkan Foundation

- [x] **1.1 Validation layers** — `VK_LAYER_KHRONOS_validation` + debug messenger in `vulkanContext.cpp`, gated on `!NDEBUG`
- [x] **1.2 Debug labels / object names** — `core/debugUtils.h`, scoped labels around framegraph passes, named buffers/images
- [x] **1.3 GPU timestamp queries** — `core/gpuTimer.h`, per-pass timing surfaced in the Profiler window
- [x] **1.4 ImGui overlay** — wired through `ui/imguiLayer`, live FPS / GPU timings / scene controls

## Now — Stage 1 wrap-up

- [ ] **1.5 Synchronization audit** — re-scoped: audit `FrameGraph` barrier derivation (`FgResourceState` transitions), confirm stage masks are tight, document the present submit's `waitDstStageMask`
- [ ] **1.6 Camera system** — extract from `main.cpp` (currently fixed perspective + Z translate); add free-fly or arcball, separate view/proj from `ShaderData` upload path
- [ ] **1.7 Tracy profiling** *(pulled forward from Phase 3)* — scoped CPU zones + GPU context; profile *before* GPU-driven work so wins are measurable
- [ ] **1.8 Threading foundation** *(NEW — foundational, see Ultimate Roadmap Tier 0)* — establish contracts up front: per-thread command pools, pick a job system (Taskflow / enkiTS / own), hand each FrameGraph pass its own recording context. Parallel pass recording filled in incrementally; **single submit thread**.

## Next — Stage 2: Renderer Architecture

- [ ] **2.1 Deletion queue** — `core/deletionQueue.h`, replace manual cleanup in `main.cpp`
- [ ] **2.2 Descriptor allocator** — pool-of-pools + `DescriptorLayoutBuilder` (current code uses a single hardcoded pool + inline layout)
- [ ] **2.3 Material system** — `MaterialTemplate` / `Material`, move Phong pipeline out of `main.cpp`
- [ ] **2.4 Shader reflection** *(optional)* — drive layouts from Slang reflection
- [x] **2.5a Render graph: data structures** — passes, resource handles, per-attachment load/store ops
- [ ] **2.5b Render graph: real dependency tracking** — replace declaration-order execution with adjacency list + topological sort (prerequisite for reordering / async compute)
- [x] **2.5c Render graph: barrier generation** — auto `VkImageMemoryBarrier2` from `FgResourceState`
- [ ] **2.5d Render graph: transient resources** — lifetime tracking + memory aliasing across non-overlapping passes
- [ ] **2.6a KTX2 loader** — pull in libktx, replace stb-via-tinygltf path; query `vkGetPhysicalDeviceFormatProperties` for BC7/BC5; upload full mip chain in one `vkCmdCopyBufferToImage`
- [ ] **2.6b Asset bake pipeline** — CMake custom target running `toktx`/`ktx create` over a manifest of inputs; commit `.ktx2` to repo (reviewers shouldn't need KTX-Software on PATH to build)
- [ ] **2.6c Mip generation strategy** — decide: offline (`toktx --genmipmap`) vs. GPU blit chain at load; document the choice. *Recommend offline for first pass.*
- [ ] **2.7 Multi-mesh scene loading** *(NEW)* — keep per-primitive transforms and material indices instead of flattening every glTF mesh into one vertex/index buffer; prerequisite for per-material PBR (3.1) and per-object culling (3.4)

> Open questions for 2.6 (decide before starting):
> - Keep glTF `images[]` array, or re-export with `KHR_texture_basisu`? *(Lean: switch — proper modern path)*
> - Do **2.2** (descriptor allocator) **first** so texture loading touches the new allocator, not the hardcoded pool

## Code Organization — incremental refactors

- [ ] **R.1 Extract `recordFrame(...)`** — pull the per-frame body out of `main.cpp` into a free function (no class). Reversible; just a readability win.
- [ ] **R.2 Move `uploadTexture` to `resources/`** — already a free function in `main.cpp`; relocate into `resources/image.cpp` (or new `resources/textureUpload.cpp`). Strict win, no design decisions.
- [ ] *(deferred)* `App`/`Renderer` class — design after 2.3 lands so it reflects real shape
- [ ] *(deferred)* `input/` module — extract together with 1.6 Camera

## Later — Stage 3: Modern Rendering

- [ ] **3.1 PBR material** — Cook-Torrance BRDF, metallic/roughness from glTF
- [ ] **3.2 IBL** — irradiance / prefilter / BRDF LUT (first compute shaders)
- [ ] **3.3 Shadow mapping** — depth-only shadow pass via render graph
- [ ] **3.4 GPU culling / indirect drawing** — compute frustum cull → `vkCmdDrawIndexedIndirectCount`
- [ ] **3.5 HDR + tonemapping** *(NEW, pulled from Phase 2)* — HDR render target, tonemap pass; PBR (3.1) looks wrong without it
- [ ] **3.6 Deferred shading** *(NEW)* — G-buffer pass on transient resources (2.5d)
- [ ] **3.7 Clustered lighting** *(NEW)* — per-cluster light assignment in compute; ms table vs forward
- [ ] **3.8 TAA** *(NEW)* — needs resources that persist across frames (2.5d)

## Backlog — Stage 4: Engine Grade

- [ ] **4.1 Full bindless resource table**
- [ ] **4.2 Async compute** (separate queue — depends on 2.5b/2.5d)
- [ ] **4.3 Hot-reload shaders**
- [ ] **4.4 Mesh shaders**
- [ ] **4.5 Ray tracing** (shadows + reflections)
- [ ] **4.6 LBVH + compute shadows** *(NEW, stretch)* — Morton codes + radix sort, software traversal in compute; hardware ray query as the reference for build and trace time

## Misc

- [ ] Setup clang-tidy
