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
| 🟢 | [core/vulkanContext](source/core/vulkanContext.cpp) | ✅ spine: `vkCreateInstance`→`vkCreateDevice`→`vmaCreateAllocator`. ✅ **features vs extensions** are different axes (features = capability toggles via `vk12`/`vk13`+`pEnabledFeatures`; extensions = API surface by name string, e.g. swapchain). ✅ features are negotiated **only at `vkCreateDevice`** — instance knows nothing about device caps; `vkEnumeratePhysicalDevices` does **not** filter (it's `ls`, not `find`); `assert` only guards the array index. ✅ **no device suitability check** → blindly takes `devices[0]`; if it lacks a feature, `vkCreateDevice`→`VK_ERROR_FEATURE_NOT_PRESENT`→`vkCheck` prints to stderr + `exit()` = hard crash with extra steps (app vanishes, no window). Safe *only* because dev box has one capable discrete GPU. ✅ surface/swapchain live **outside** by responsibility **and** lifetime (device is created once & immutable; swapchain rebuilds on resize; surface needs a third-party-windowed OS handle). ✅ two-call enumerate idiom (count, then fill). Minor: `vkCheck` treats `VK_INCOMPLETE` as fatal (only tests `!=VK_SUCCESS`) — latent, can't trigger today (count-first idiom); **ticketed: Notion 1.10**. |
| 🟢 | [core/swapchain](source/core/swapchain.cpp) | ✅ **format hardcoded** `B8G8R8A8_SRGB`+`SRGB_NONLINEAR` with no `vkGetPhysicalDeviceSurfaceFormatsKHR` query → same blind-`devices[0]` pattern; unsupported pair = garbage/validation error, "safe" only because dev surface happens to expose it. ✅ **image count** passes `caps.minImageCount` straight → `+1` is the best-practice fix, and the *real why* is present-engine ownership: with exactly the min, the engine holds one for scanout and `vkAcquireNextImageKHR` can find none free → CPU stalls waiting on the next image (frame-pacing hitch), not "1 is too few". ✅ **extent** built from window `size`, ignores `caps.currentExtent`: the `0xFFFFFFFF` sentinel = "swapchain, you choose"; a *concrete* value = compositor dictates the size (e.g. Wayland) and ignoring it is wrong/validation error. Also unclamped to min/maxImageExtent. Surface-caps fix **ticketed: Notion 1.8**. ✅ **present mode FIFO** = always-available safe choice (no mailbox). ✅ **recreate ordering**: destroys old swapchain immediately after creating new (L55); spec forbids destroy while acquired images have outstanding ops → legal **only** because the *caller* (main.cpp:523) does `vkDeviceWaitIdle` first. Precondition is caller-enforced & undocumented = fragility; that one idle is **amortized** across swapchain + depth recreate, so burying it in `recreate` isn't a clean win. Fix **ticketed: Notion 1.7**. |
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
- **Swapchain hardcodes format + ignores surface caps.** No `vkGetPhysicalDeviceSurfaceFormatsKHR` query (format/colorspace assumed), `caps.currentExtent` ignored (breaks on size-dictating compositors like Wayland), extent unclamped to min/max, `minImageCount` used without `+1`. Portability + frame-pacing milestone, same family as the device-suitability gap. **Ticketed: Notion 1.8** (surface caps) + **1.7** (recreate device-idle precondition).
- **No physical-device suitability check.** `vulkanContext` takes `devices[0]` and requests a fat feature set with zero validation; a non-capable device 0 → `vkCreateDevice` fails → `exit()` deep in init (no window, cryptic stderr). Safe only on a single-capable-GPU dev box. Real fix: enumerate → score with `vkGetPhysicalDeviceFeatures2`/`Properties2`, pick the qualifying device, fail gracefully if none. Portability milestone, not a bug today. **Ticketed: Notion 1.9.**

## Reference

- Book for the *architecture* phase: *Mastering Graphics Programming with Vulkan*
  (assumes fundamentals — which Caldera already has). Read for patterns, implement in
  Caldera's own abstractions; steal architecture, not code.
