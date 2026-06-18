#include "core/log.h"

#include <cstdarg>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  // OutputDebugStringA

#include "minilog.h"

namespace caldera {
namespace {

// minilog writes to the console and the log file but not the debugger Output
// window. Registered for every level in logInit(), this tees each message there
// too, so it shows up in the Visual Studio / VS Code debug pane. minilog strips
// the trailing newline before invoking callbacks, so we add it back.
void teeToDebugger(void* /*userData*/, const char* msg) {
  OutputDebugStringA(msg);
  OutputDebugStringA("\n");
}

}  // namespace

void logInit() {
  minilog::LogConfig cfg{};
  cfg.forceFlush = true;      // flush after every line so a crash loses nothing
  cfg.coloredConsole = true;  // severity colors in the console
  cfg.threadNames = true;     // prefix each line with the logging thread's name
  minilog::initialize("caldera.log", cfg);

  minilog::LogCallback cb{};
  for (auto& fn : cb.funcs)
    fn = &teeToDebugger;
  minilog::callbackAdd(cb);
}

void logShutdown() {
  minilog::deinitialize();
}

void logInfo(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  minilog::logVAList(minilog::Log, fmt, args);
  va_end(args);
}

void logWarn(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  minilog::logVAList(minilog::Warning, fmt, args);
  va_end(args);
}

void logError(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  minilog::logVAList(minilog::FatalError, fmt, args);
  va_end(args);
}

}  // namespace caldera
