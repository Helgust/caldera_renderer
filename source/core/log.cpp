#include "core/log.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  // OutputDebugStringA

namespace caldera {

// One reusable buffer, like raptor's log_buffer — the renderer is single
// threaded on the log path.
static char gLogBuffer[8192];

static void teeToFile(const char* text) {
  // Best-effort: a failed log write must never mask the original fault.
  FILE* f = nullptr;
  if (fopen_s(&f, "caldera.log", "a") != 0 || !f)
    return;
  std::time_t now = std::time(nullptr);
  std::tm tm{};
  char stamp[32];
  if (localtime_s(&tm, &now) == 0 &&
      std::strftime(stamp, sizeof(stamp), "%H:%M:%S", &tm) > 0)
    std::fprintf(f, "[%s] %s", stamp, text);
  else
    std::fputs(text, f);
  std::fclose(f);
}

void logMessage(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(gLogBuffer, sizeof(gLogBuffer), fmt, args);
  va_end(args);
  gLogBuffer[sizeof(gLogBuffer) - 1] = '\0';

  std::fputs(gLogBuffer, stdout);  // console
  std::fflush(stdout);
  OutputDebugStringA(gLogBuffer);  // debugger Output window
  teeToFile(gLogBuffer);           // caldera.log
}

}  // namespace caldera
