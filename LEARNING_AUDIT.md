# Caldera — Comprehension Audit

A module-by-module "rewind" of the renderer. Much of this code was adapted from a
Vulkan tutorial base and split up with AI help under time pressure, so the goal here
is to **back-fill ownership**: understand code that already works.

## How to use this

For each module, before reading it closely, try to answer from memory:

1. **Responsibility** — what is this module's job, in one sentence?
2. **Why separate** — what would break / get worse if I inlined it?
3. **Core calls** — the 2–3 Vulkan calls at its heart, and *why those*?
4. **Decisions** — what did I (or the AI) choose here that could've gone differently?

If I can't answer → that's the module to study next. Work with Claude as a Socratic
checker: explain a module in my own words, Claude probes the thin spots and points at
exact lines. **I do the thinking; Claude finds the holes.** No hand-fed answers.

Status legend: 🔴 not started · 🟡 in progress · 🟢 understood & can explain unaided

## Modules

| Status | Module | Notes / open questions |
|---|---|---|
| 🔴 | [core/vulkanContext](source/core/vulkanContext.cpp) | instance, physical/logical device, queues, VMA, feature enablement |
| 🔴 | [core/swapchain](source/core/swapchain.cpp) | surface caps, format/present-mode choice, recreate path |
| 🟢 | [core/syncObjects](source/core/syncObjects.h) | ✅ semaphores = GPU↔GPU ordering, fences = GPU→CPU resource reuse; ✅ derived cold that `fence[frameIndex]` covers frame **N−2** not N−1 → an in-flight frame's writes to a *shared* resource aren't protected. Fix for the shared depth image is **ticketed: Notion 1.6** (per-frame depth ring). |
| 🔴 | [core/commandManager](source/core/commandManager.h) | pools/buffers, frames-in-flight, `submitOneTime` |
| 🔴 | [core/debugUtils](source/core/debugUtils.h) | object naming, debug labels, validation setup |
| 🔴 | [core/gpuTimer](source/core/gpuTimer.h) | timestamp queries, readback timing, pool reset rules |
| 🟡 | [rendering/framegraph](source/rendering/framegraph.cpp) | ✅ barriers = exec + memory + layout; ✅ state tracking via `r.state`; ✅ understand *why* state doesn't persist across frames (graph rebuilt each frame, `importImage` resets to `{}`). OPEN (→ green): trace how a graph would **allocate + alias** transient resources, not just import them. Stage 2 work. |
| 🔴 | [rendering/pipeline](source/rendering/pipeline.cpp) | builder pattern, dynamic state, dynamic rendering formats |
| 🔴 | [rendering/shader](source/rendering/shader.cpp) | Slang compile → SPIR-V → module |
| 🔴 | [resources/buffer](source/resources/buffer.cpp) | VMA usage, host-visible vs device-local, BDA |
| 🔴 | [resources/image](source/resources/image.cpp) | create2D, views, `transitionLayout` barrier construction |
| 🔴 | [scene/sceneLoader](source/scene/sceneLoader.cpp) | glTF parse → vertices/indices/images; current single-mesh limit |
| 🔴 | [ui/imguiLayer](source/ui/imguiLayer.cpp) | ImGui Vulkan backend init, its descriptor pool, draw integration |
| 🔴 | [main.cpp](source/main.cpp) | orchestration: descriptor set (bindless textures), BDA push, render loop, input |

## Known architectural gaps (future milestones, not bugs today)

- **Frame graph: transient + persistent resources.** Today only `importImage`; resources
  are externally owned and state resets to `{}` each frame. Blocks: producer→consumer
  passes (shadow map, G-buffer) and temporal techniques (TAA, reprojection) that need
  last frame's contents to survive.
- **Full bindless.** Textures use descriptor indexing, but per-draw still binds a
  descriptor set + pushes BDA. Consolidate toward one set / addresses.
- **Multiple meshes / materials / draw batching.** `scene.mesh` is singular today.
- **GPU-driven rendering.** Plain `vkCmdDrawIndexed` → indirect + culling later.
- **Single depth image vs frames-in-flight** — ✅ understood (real WAW/WAR hazard; fence covers N−2, not N−1). Fix **ticketed: Notion 1.6** (per-frame depth ring).

## Reference

- Book for the *architecture* phase: *Mastering Graphics Programming with Vulkan*
  (assumes fundamentals — which Caldera already has). Read for patterns, implement in
  Caldera's own abstractions; steal architecture, not code.
