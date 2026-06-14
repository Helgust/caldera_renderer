#include "core/assert.h"
#include <cstdlib>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  // IsDebuggerPresent / MessageBoxA

namespace caldera {

void breakIfDebugger() {
  if (IsDebuggerPresent())
    CALDERA_DEBUG_BREAK();
}

void onCheckFailed(const char* summary) {
  if (IsDebuggerPresent())
    return;  // let the caller's CALDERA_DEBUG_BREAK() stop on the failing line

  // No debugger: a raw break would just crash. Hold a modal open instead so the
  // message — and the console window — survive until dismissed, then exit.
  MessageBoxA(nullptr, summary ? summary : "Fatal error",
              "Caldera - Fatal Error",
              MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
  std::exit(EXIT_FAILURE);
}

}  // namespace caldera
