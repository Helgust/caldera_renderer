#pragma once

#include "core/log.h"

// Assert / fatal handling, modeled on raptor's foundation/assert.hpp from
// "Mastering Graphics Programming with Vulkan": RASSERT / RASSERTM print a
// "file(line) : message" string then RAPTOR_DEBUG_BREAK so you land on the
// failing call site. Output goes through caldera::logError() (see log.h), so
// every failure also reaches the debugger Output window and caldera.log.

// --- file(line) prefix, matching raptor's RAPTOR_FILELINE --------------------
#define CALDERA_STRINGIZE(x) #x
#define CALDERA_MAKESTRING(x) CALDERA_STRINGIZE(x)
#define CALDERA_FILELINE(msg) __FILE__ "(" CALDERA_MAKESTRING(__LINE__) ") : " msg

// --- debugger break ----------------------------------------------------------
#if defined(_MSC_VER)
  #define CALDERA_DEBUG_BREAK() __debugbreak()
#elif defined(__has_builtin) && __has_builtin(__builtin_debugtrap)
  #define CALDERA_DEBUG_BREAK() __builtin_debugtrap()
#else
  #include <csignal>
  #define CALDERA_DEBUG_BREAK() raise(SIGTRAP)
#endif

namespace caldera {

// Break into the debugger if one is attached; otherwise a no-op. For non-fatal
// "stop and look" points (e.g. a validation ERROR) where you want to inspect
// state but keep running.
void breakIfDebugger();

// Called by the assert macros after the message has been logged. When a
// debugger is attached this returns immediately so the macro's CALDERA_DEBUG_
// BREAK() fires at the call site. With no debugger it shows a modal dialog
// (active pause), then terminates — so the raw break never crashes silently.
void onCheckFailed(const char* summary);

}  // namespace caldera

// Assert that stays live in every build (unlike <cassert>, which NDEBUG strips).
#define CALDERA_ASSERT(cond)                                          \
  do {                                                                \
    if (!(cond)) {                                                    \
      ::caldera::logError(CALDERA_FILELINE("Assertion failed: " #cond)); \
      ::caldera::onCheckFailed("Assertion failed: " #cond);          \
      CALDERA_DEBUG_BREAK();                                          \
    }                                                                 \
  } while (0)

// Assert with a printf-style message (fmt must be a string literal).
#define CALDERA_ASSERT_MSG(cond, fmt, ...)                                 \
  do {                                                                     \
    if (!(cond)) {                                                         \
      ::caldera::logError(CALDERA_FILELINE(fmt) __VA_OPT__(, )             \
                            __VA_ARGS__);                                  \
      ::caldera::onCheckFailed(fmt);                                       \
      CALDERA_DEBUG_BREAK();                                               \
    }                                                                      \
  } while (0)
