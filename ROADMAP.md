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
* [ ] Validation layers
* [x] Physical device selection
* [x] Logical device creation
* [] Queue management

## Window & Swapchain

* [ ] Surface creation
* [ ] Swapchain creation
* [ ] Image views
* [ ] Framebuffers
* [ ] Resize handling

## Command System

* [ ] Command pool management
* [ ] Command buffer recording
* [ ] Frame synchronization (fences/semaphores)

## Resource Management

* [ ] GPU buffer abstraction
* [ ] Image abstraction
* [ ] Memory allocation strategy
* [ ] Descriptor pool allocator

## Pipeline System

* [ ] Pipeline builder abstraction
* [ ] Shader module loader
* [ ] Descriptor set layout system
* [ ] Graphics pipeline creation

## Scene Rendering

* [ ] Mesh loading (GLTF)
* [ ] Camera system
* [ ] Basic scene graph (minimal)
* [ ] Depth buffer

## Lighting

* [ ] Directional light
* [ ] Basic shadow mapping

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

* [ ] Frame graph / render graph
* [ ] Render pass abstraction
* [ ] Resource lifetime management

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

* [ ] GPU timestamp queries
* [ ] CPU/GPU profiling
* [ ] Frame time breakdown
* [ ] Pipeline statistics

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
