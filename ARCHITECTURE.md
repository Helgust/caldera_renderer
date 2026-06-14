# Caldera — Architecture

Front door to the renderer. Read this when returning to the project cold.
For per-module deep dives see [LEARNING_AUDIT.md](LEARNING_AUDIT.md); for
ticketed defects and roadmap items see the Notion "Caldera Renderer —
Development Roadmap" board.

This document captures **what the audit derived that the code does not state
on its own** — cross-cutting invariants, load-bearing decisions, and the
reasoning behind them. The subsystem tour belongs in the audit; here we keep
only what survives a cold reread.

## Frame in 60 seconds

Per frame in [main.cpp:339-532](source/main.cpp#L339-L532):

1. `waitAndResetFence(frameIndex)` — block until this slot's *previous* GPU
   submit has retired (the N−2 rule, below).
2. `vkAcquireNextImageKHR` — obtain a swapchain image index; signals
   `presentSemaphores[frameIndex]` when the image is actually free for write.
3. CPU writes per-frame uniforms (`projection`, `view`, `model[3]`, light,
   selection) into `shaderDataBuffers[frameIndex]` — a host-visible, BDA-addressable
   ring slot, safe to overwrite because the fence above retired it.
4. Build ImGui widgets (immediate-mode — `Begin/Slider/...` builds draw lists).
5. Begin the per-frame command buffer; `gpuTimer.beginFrame(cb, frameIndex)`
   reads back the previous results for this slot and resets its query range
   (must happen **outside** any render-pass scope — spec rule for
   `vkCmdResetQueryPool`).
6. Stack-construct `FrameGraph`, import the swapchain image and depth, declare
   passes (`forward`, `imgui`, `present`), then `execute`. The graph computes
   barriers from declared usages, drives `vkCmdBeginRendering`/`EndRendering`,
   and brackets each pass with gpuTimer timestamps.
7. `vkQueueSubmit` — waits on `presentSemaphores[frameIndex]` at
   `COLOR_ATTACHMENT_OUTPUT`, signals `renderSemaphores[imageIndex]` + the
   per-frame fence.
8. `vkQueuePresentKHR` — waits on `renderSemaphores[imageIndex]`.
9. Poll SDL events (one frame late w.r.t. ImGui input — see Known limitations).
   Set `updateSwapchain` if acquire/present returned `OUT_OF_DATE` or a
   `WINDOW_RESIZED` event arrived.
10. If flagged: `vkDeviceWaitIdle` → recreate swapchain + depth. Pipeline,
    descriptors, and shader-data buffers survive (see invariant 4).

## Invariants

### 1. The N−2 fence rule

`fence[frameIndex]` covers frame **N−2**, not N−1. Any resource indexed by
`[frameIndex]` — the command buffer, the shader-data ring slot, the gpuTimer
query range — is safe to reset or overwrite *because* the fence proves that
slot's previous GPU submit retired.

**Corollary:** any resource *shared* across frames in flight (the single
depth image) is **not** protected — its frame-N−1 writes may still be in
flight when frame N begins. The single-depth-image WAW hazard (ticketed
Notion 1.6) is the cleanest example.

Touches: [syncObjects.h](source/core/syncObjects.h),
[commandManager.h](source/core/commandManager.h),
[gpuTimer.h](source/core/gpuTimer.h),
[buffer.cpp](source/resources/buffer.cpp),
[main.cpp](source/main.cpp).

### 2. Layout is timeline-positional, not an object property

An image's layout is a fact about a *point on the GPU timeline*, not about
the `Image` object. The shared depth image is simultaneously in two layouts
for frame N and N−1, so the `Image` struct deliberately stores no
`currentLayout` ([image.cpp](source/resources/image.cpp)).

Division of labor:
- `Image::transitionLayout` is the **dumb verb** — performs the `old→new`
  barrier with whatever it's told.
- `FrameGraph` is the **bookkeeper** — `r.state` tracks
  `(layout, stage, access)` per resource, computes `oldLayout` from the last
  use, calls `transitionLayout`, writes back the new state
  ([framegraph.cpp](source/rendering/framegraph.cpp)).

This is the job `VkRenderPass` used to do implicitly (via
`initial/finalLayout` + subpass dependencies) before dynamic rendering moved
it to the application.

### 3. "Alive at the synchronous read," not "member vs local"

All Vulkan `*CreateInfo` structs and their indirect pointers (`pNext`,
`pStages`, `pAttachments`, `pImageInfo`, ...) are consumed **synchronously
during the create call**. Vulkan retains nothing after the call returns.

Practical consequences:
- `GraphicsPipelineBuilder` can keep most fields as locals or members
  interchangeably — what matters is they're alive when `build()` runs the
  create, not afterward ([pipeline.cpp](source/rendering/pipeline.cpp)).
- `VkShaderModule` is **transient**: once `vkCreateGraphicsPipelines` has
  consumed its SPIR-V, the module can be destroyed immediately
  ([shader.cpp](source/rendering/shader.cpp)).
- `VkWriteDescriptorSet::pImageInfo` must be alive at `vkUpdateDescriptorSets`,
  not at draw time.

The rule generalizes: **"member = safe, local = dangerous"** is the wrong
mental model; lifetime must cover the *call*, no more.

### 4. Dynamic state vs baked

A `VkPipeline` is a pre-baked hardware blob: shader code, vertex layout,
blend, depth, attachment formats are compiled in at
`vkCreateGraphicsPipelines`. Binding at draw time is a cheap pointer/register
swap.

State declared **dynamic** (viewport, scissor —
[pipeline.cpp:71](source/rendering/pipeline.cpp#L71)) is set per draw, paying
per-draw cost to dodge a rebuild spike. The choice is **bake what's stable,
dynamic only what varies often.**

This is what makes **resize cheap**: viewport and scissor are dynamic, so a
swapchain extent change only rebuilds the swapchain and depth — never the
pipeline. The dynamic-state choice in the pipeline *is* the resize plumbing.

### 5. "Works ≠ correct" — the unfalsified-defect family

Several defects are latent because *untested*, not because they don't exist:

- Device selection — ✅ selection fixed (CLR-33 / 1.9; discrete-preferred
  enumeration, no more blind `devices[0]`), but the requested feature chain is
  still unvalidated → `VK_ERROR_FEATURE_NOT_PRESENT` crash on a non-capable
  device remains until CLR-44 / 1.21
  ([vulkanContext.cpp](source/core/vulkanContext.cpp), Notion 1.9, 1.21)
- Hardcoded swapchain format + ignored surface caps — ✅ fixed (CLR-32 / 1.8;
  surface-format query + extent clamp + min+1 image count)
  ([swapchain.cpp](source/core/swapchain.cpp), Notion 1.7, 1.8)
- gpuTimer param-shadows-member (was fixed; Notion 1.13)
- Slang null-module dereference on shader error
  ([shader.cpp:32-43](source/rendering/shader.cpp#L32-L43), Notion 1.14)
- `Vertex` hash / `operator==` contract mismatch in sceneLoader
  ([sceneLoader.h](source/scene/sceneLoader.h), Notion 1.15)
- uint16 index truncation on large meshes
  ([sceneLoader.cpp:80-81](source/scene/sceneLoader.cpp#L80-L81), Notion 1.16)
- `Buffer::upload` missing flush + missing `dstOffset` + null `pMappedData`
  trap ([buffer.cpp](source/resources/buffer.cpp), Notion 1.19)
- `Image::create2D` forces `DEDICATED_MEMORY_BIT` per image
  ([image.cpp:25](source/resources/image.cpp#L25), Notion 1.18)
- Acquire `OUT_OF_DATE` fall-through → submit on unsignaled semaphore
  ([main.cpp:345-349](source/main.cpp#L345-L349), Notion 1.20)

**Common shape:** dev box + forgiving driver + validation off → the bug
never fires → "works." Validation on (set `wantValidation=true` at
[vulkanContext.cpp:49](source/core/vulkanContext.cpp#L49)) is mandatory
before trusting any module. Several of these were caught only by flipping
validation on after the audit row was written.

## Load-bearing decisions

| Decision | Alternative | Why this one |
|---|---|---|
| **Dynamic rendering** (`VkPipelineRenderingCreateInfo`, no `VkRenderPass` / `VkFramebuffer` / subpasses) | Render-pass objects + subpasses | VK 1.3 baseline; the framegraph rebuilds the dependency DAG each frame anyway, so subpasses' compile-time graph is redundant. Cost = lose subpass tile-shading hints (not a target). |
| **BDA push + per-frame ring** for uniforms (push = 8 B device address, payload ≈ 340 B in `shaderDataBuffers[frameIndex]`) | UBO descriptor set | Push is hard-capped at 128 B guaranteed → can't fit `ShaderData`; descriptors add rebind churn + pool/layout plumbing. BDA hands the shader a raw pointer for free. Ring slot protected by N−2 fence. |
| **Bindless textures** — one `COMBINED_IMAGE_SAMPLER` array sized at scene load, shader picks by index | One descriptor set per material | Single descriptor bind for the whole forward pass; expanding the material set is appending to the array. Today's cost = per-texture sampler duplication (split-sampler idiom not yet adopted — known gap). |
| **Single shared depth image** | Per-frame depth ring | Simplest; defers the WAW hazard until depth is actually sampled across frames. Known gap — ticketed Notion 1.6. |
| **VMA** for buffer/image allocation | Manual `VkDeviceMemory` + bind calls | Sub-allocation, alignment, fragmentation — solved. Current policy under-uses it: `Image::create2D` forces dedicated allocations per image; `Buffer::createGpuOnly` is written but never called (geometry lives in host-visible memory). Ticketed Notion 1.18, 1.19. |
| **Slang → SPIR-V at app start** | GLSL via `glslang` at app start; pre-baked SPIR-V at build time | Slang gives one IR feeding multiple back-ends (SPIR-V today, DXIL/Metal/HLSL later). Build-time SPIR-V is a future optimization, not an architectural choice. |
| **Stack-scoped `FrameGraph` per frame** | Persistent framegraph rebuilt in-place | Simplest ownership (RAII destroys at end of frame); passes and resource state rebuilt fresh each frame. Cost = `createImage` is unsafe in the loop (per-frame `vmaCreateImage`/destroy), which is why transient-pool + aliasing barriers are blocked on a redesign. |
| **Single command pool**, per-buffer reset via `RESET_COMMAND_BUFFER_BIT` | One pool per frame-in-flight + whole-pool reset | Simplest correct option while recording is single-threaded. `vkResetCommandPool` would reset *all* in-flight slots at once and the fence only proves the current slot is done. Scales worse but is right for now. |

## Known limitations (not bugs today)

Listed in detail at the bottom of [LEARNING_AUDIT.md](LEARNING_AUDIT.md);
summarized here as architectural-gap milestones rather than per-module
findings:

- **Transient pool + aliasing barriers.** `createImage` is unimplemented in
  practice; per-frame allocation in the loop would churn. Unblocks
  producer→consumer passes (shadow map, G-buffer) and cross-frame
  temporal techniques (TAA).
- **Full bindless.** Textures are bindless; per-draw still binds a descriptor
  set and pushes a BDA. Consolidate toward one global set + addresses.
- **Texture upload policy.** Dedicated allocation per image marches toward
  `maxMemoryAllocationCount`. Maturity ladder: drop the blanket flag → pool
  for materials, dedicated for render targets → batch staging → KTX2/BCn
  with offline mips.
- **Geometry in host-visible memory.** GPU fetches vertex/index data across
  PCIe every draw. `Buffer::createGpuOnly` exists; the staging path doesn't
  use it yet.
- **Single-mesh scene loader.** All glTF meshes flatten into one vertex/index
  buffer; per-primitive transforms and material binding are lost. Ties to
  multi-draw / GPU-driven rendering.
- **ImGui input is one frame late.** `NewFrame` runs at loop top, events poll
  at loop bottom (Notion 1.17). Latency, not correctness.

## Pointers

- **Per-module deep dives** → [LEARNING_AUDIT.md](LEARNING_AUDIT.md)
- **Defect & roadmap tracker** → Notion "Caldera Renderer — Development
  Roadmap" (tickets 1.6–1.20 at time of writing)
- **Architecture-phase reading** — *Mastering Graphics Programming with
  Vulkan*. Read for patterns; implement in Caldera's own abstractions. Steal
  architecture, not code.
