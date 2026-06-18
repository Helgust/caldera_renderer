#pragma once

// Logging facade over minilog (corporateshark/minilog) -- a small, thread-safe
// logger. We adopt it ahead of the renderer going multithreaded (async asset
// streaming, a job system): unlike the previous single-threaded sink, every
// minilog call is safe to make from any thread and is tagged with its thread
// name.
//
// minilog writes to a colored console and a log file, but NOT to the debugger
// Output window. logInit() registers a callback that tees every message to
// OutputDebugStringA, preserving the old guarantee that a message is never lost
// even when the console is gone (e.g. a RenderDoc-launched run whose window
// closed): console + debugger Output window + caldera.log all receive it.
//
// The caldera:: wrappers keep minilog.h out of the rest of the codebase, so the
// logger stays swappable from this one translation unit.

namespace caldera {

// Start/stop the logger. logInit() opens caldera.log (truncated each run) and
// installs the debugger tee; it must run once before any log call. logShutdown()
// flushes and closes the file. Logging before logInit() still reaches the
// console (just not the file), so early asserts are never silent.
void logInit();
void logShutdown();

// printf-style sinks at increasing severity. Do NOT pass a trailing '\n' --
// minilog appends one. The console shows Info and above; the file records all.
void logInfo(const char* fmt, ...);
void logWarn(const char* fmt, ...);
void logError(const char* fmt, ...);

}  // namespace caldera
