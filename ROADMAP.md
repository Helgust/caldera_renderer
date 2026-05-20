# Vulkan Renderer Roadmap

This document describes the development roadmap for the Vulkan renderer project.
The goal of this repository is to serve as a **modern real-time rendering sandbox** demonstrating strong graphics programming, GPU knowledge, and modern C++ architecture.

The project is intended to showcase the following competencies:

* Vulkan API expertise
* Real-time rendering techniques
* GPU optimization strategies
* Clean engine architecture
* Modern C++ practices

This roadmap is divided into **4 major phases** that correspond roughly to development quarters.

> **Detailed implementation tickets** (per-step goals, target files, verification criteria) live in
> the Notion board: *Caldera Renderer — Development Roadmap*. Items tagged `CLR — Step X.Y` below
> map 1:1 to those tickets. `TODO.md` holds the active near-term backlog.

---

# Project Goals

Primary goals:

* Build a **modern Vulkan renderer**
* Implement **core real-time rendering techniques**
* Demonstrate **GPU architecture understanding**
* Maintain **clean, modular C++ design**
* Produce **portfolio-quality code**

Non-goals:

* Full game engine
* Editor
* Gameplay systems
* Asset pipeline complexity

---

# Phase 1 — Foundation

Goal: Build a stable Vulkan rendering base and minimal scene rendering.

Estimated timeline: 2–3 months.

## Core Vulkan Setup

* [x] Instance creation
* [x] Validation layers (CLR — Step 1.1; `VK_LAYER_KHRONOS_validation` + debug messenger, gated on `!NDEBUG`)
* [x] Physical device selection
* [x] Logical device creation (Vulkan 1.2/1.3 feature chains)
* [x] Queue management (graphics queue family selection)

## Window & Swapchain

* [x] Surface creation (SDL3)
* [x] Swapchain creation (FIFO present mode)
* [x] Image views
* [x] Framebuffers (using dynamic rendering — no VkFramebuffer needed)
* [x] Resize handling (swapchain recreation)

## Command System

* [x] Command pool management (per-frame, RESET_COMMAND_BUFFER)
* [x] Command buffer recording
* [x] Frame synchronization (fences/semaphores, N=2 in flight)

## Resource Management

* [x] GPU buffer abstraction (VMA, host-visible & GPU-only, BDA)
* [x] Image abstraction (create2D, Synchronization2 layout transitions)
* [x] Memory allocation strategy (VMA)
* [ ] Descriptor pool allocator (CLR — Step 2.2; currently hardcoded single pool)

## Pipeline System

* [x] Pipeline builder abstraction (fluent GraphicsPipelineBuilder)
* [x] Shader module loader (Slang → SPIR-V at runtime)
* [ ] Descriptor set layout system (CLR — Step 2.2; currently inline in main.cpp)
* [x] Graphics pipeline creation (dynamic rendering)

## Scene Rendering

* [x] Mesh loading (GLTF via tinygltf, vertex dedup)
* [ ] Camera system (CLR — still fixed perspective + translate in main.cpp; only Z slider exposed via ImGui)
* [ ] Basic scene graph (minimal)
* [x] Depth buffer

## Lighting

* [x] Directional light (Phong, hardcoded in shader.slang)
* [ ] Basic shadow mapping (CLR — Step 3.3)

## Tooling & Debugging (added per plan)

* [x] Debug labels / object names (CLR — Step 1.2; `debugUtils.h` + scoped labels around passes)
* [x] GPU timestamp queries (CLR — Step 1.3; `core/gpuTimer.h`, per-pass timing surfaced in Profiler window)
* [x] ImGui overlay integration (CLR — Step 1.4; live FPS, GPU timings, scene controls)
* [ ] Synchronization audit & tightening (CLR — Step 1.5; mostly superseded by framegraph auto-barriers — re-scope to: audit FG barrier derivation + present submit waitStage)

---

# Phase 2 — Modern Rendering Techniques

Goal: Implement commonly used techniques in modern engines.

Estimated timeline: 3–4 months.

## Physically Based Rendering

* [ ] Metal/Rough PBR
* [ ] BRDF implementation
* [ ] Image-based lighting
* [ ] Environment maps
* [ ] Prefiltered cubemaps
* [ ] BRDF LUT

## Rendering Architecture

* [ ] Resource manager / deletion queue (CLR — Step 2.1)
* [ ] Descriptor allocator + layout builder (CLR — Step 2.2)
* [ ] Material system (CLR — Step 2.3)
* [ ] Shader reflection (CLR — Step 2.4, optional)
* [x] Render graph: data structures (CLR — Step 2.5a; passes, resource handles, per-attachment load/store ops)
* [~] Render graph: resource/dependency tracking (CLR — Step 2.5b; runs in declaration order with last-state tracking — no adjacency list / topo sort yet)
* [x] Render graph: automatic barrier generation (CLR — Step 2.5c; `VkImageMemoryBarrier2` derived from prior `FgResourceState`)
* [ ] Render graph: transient resource aliasing (CLR — Step 2.5d; transient images allocated but no lifetime overlap reuse)

## Lighting Improvements

* [ ] Cascaded shadow maps
* [ ] Shadow filtering

## Post Processing

* [ ] HDR rendering
* [ ] Tonemapping
* [ ] Gamma correction

---

# Phase 3 — Advanced Rendering

Goal: Demonstrate deeper GPU knowledge and performance techniques.

Estimated timeline: 3–4 months.

## Screen Space Techniques

* [ ] SSAO
* [ ] SSAO blur
* [ ] Temporal Anti-Aliasing (TAA)

## GPU Driven Rendering

* [ ] GPU frustum culling
* [ ] Compute based instance culling
* [ ] Indirect drawing

## Lighting Scalability

* [ ] Forward+ or clustered lighting
* [ ] Light culling on GPU

## Performance

* [x] GPU timestamp queries *(landed in Phase 1; listed here for completeness)*
* [ ] CPU/GPU profiling (Tracy or similar — scoped CPU zones, GPU context)
* [ ] Frame time breakdown (rolling history graph in ImGui)
* [ ] Pipeline statistics (`VK_QUERY_TYPE_PIPELINE_STATISTICS`)

## Async Compute (optional)

* [ ] Compute queue usage
* [ ] Async post-processing

---

# Phase 4 — Polish & Portfolio

Goal: Prepare the project as a strong portfolio piece.

Estimated timeline: 1–2 months.

## Code Quality

* [ ] Refactor core modules
* [ ] Remove temporary code
* [ ] Improve naming consistency
* [ ] Documentation comments

## Documentation

* [ ] Architecture documentation
* [ ] Rendering pipeline diagram
* [ ] GPU frame breakdown

## Portfolio Content

* [ ] Demo video
* [ ] Feature showcase scenes
* [ ] Performance analysis

## Repository Cleanup

* [ ] Improve README
* [ ] Example scenes
* [ ] Build instructions

---

# Stretch Goals

These features are optional but valuable for demonstrating advanced knowledge.

* [ ] Vulkan ray tracing reflections
* [ ] Bindless resources
* [ ] Mesh shaders
* [ ] GPU driven rendering pipeline
* [ ] Virtual shadow maps

---

# Learning Resources

The following resources guide the implementation strategy:

* *Real-Time Rendering (4th Edition)*
* *Physically Based Rendering (PBRT v4)*
* *GPU Zen series*
* Vulkan specification
* Graphics developer roadmap

---

# Long-Term Vision

This project should evolve into a **clean, well-documented rendering playground** that demonstrates:

* modern real-time rendering knowledge
* strong Vulkan expertise
* production-level engineering practices

The repository should remain **focused on rendering**, avoiding unnecessary engine complexity.
