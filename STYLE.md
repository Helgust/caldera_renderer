# Caldera — Naming Conventions

The house style. Not Google: it's a camelCase dialect that shares Google's
PascalCase types, lowercase namespaces, and ALL_CAPS macros, and diverges
deliberately everywhere else. When in doubt, match the surrounding code.

## Rules

| Element | Convention | Example |
|---|---|---|
| Class / struct / enum type | `PascalCase` | `VulkanContext`, `FrameGraph`, `FgUsage` |
| Type alias / using / typedef | `PascalCase` | `RecordFn`, `FgResource` |
| Namespace | `lowercase` | `caldera` |
| Function / method | `camelCase` | `submitOneTime()`, `create2D()` |
| Local variable | `camelCase` | `frameIndex`, `windowSize` |
| **Function parameter** | **`snake_case`** | `frame_index`, `swapchain_image_count` |
| Public / struct data member | `camelCase` | `physicalDevice`, `frameFences` |
| Private class member | `camelCase` + trailing `_` | `resources_`, `device_`, `lastResults_` |
| Enum value | `ALL_CAPS` | `COLOR_ATTACHMENT`, `PRESENT` |
| Constant / constexpr | `ALL_CAPS` | `MAX_FRAMES_IN_FLIGHT` |
| Macro | `ALL_CAPS` | `CALDERA_ASSERT` |
| Global variable | `g` + `PascalCase` | `gDebugUtilsEnabled` |
| File name | `camelCase` | `vulkanContext.h`, `sceneLoader.cpp` |

## The one rule that makes this distinctive

**Parameters are `snake_case`; locals are `camelCase`.** Inside a function body
you can tell at a glance what came in as an argument versus what was declared
locally:

```cpp
void GpuTimer::init(VulkanContext& ctx, uint32_t frames_in_flight,   // params
                    uint32_t max_passes_per_frame) {
  uint32_t queryCount = frames_in_flight * max_passes_per_frame * 2; // local
  ...
}
```

## Note on ALL_CAPS

Enum values, constants, and macros all use `ALL_CAPS`, so they're not
distinguishable from one another by name alone — rely on scope
(`FgUsage::COLOR_ATTACHMENT`) and context. This is intentional and matches the
games-industry norm.

## Quick reference: what differs from Google

Everywhere Google uses `snake_case` or `PascalCase`-for-functions, we use
`camelCase` instead — except **parameters**, which match Google's `snake_case`.
Shared with Google: PascalCase types, lowercase namespaces, ALL_CAPS macros,
and the trailing `_` on private members (on a camelCase body).
