#pragma once

// Logging sink, modeled on the two reference renderers:
//   * "Mastering Graphics Programming with Vulkan" (raptor): rprint fans output
//     to the console and the debugger Output window via OutputDebugStringA.
//   * "3D Graphics Rendering Cookbook" / lightweightvk: minilog also tees to a
//     log file.
// We do both, so a message is never lost even when the console is gone (e.g. a
// RenderDoc-launched run whose window closed): logMessage() writes to the
// console, the debugger Output window, AND caldera.log.

namespace caldera {

// printf-style sink: console + debugger Output window + caldera.log.
void logMessage(const char* fmt, ...);

}  // namespace caldera
