@echo off
REM ============================================================================
REM  compile_shaders.bat - compile/validate every .slang under shaders\
REM
REM  The flags below MUST mirror the in-app Slang session created in
REM  source/rendering/shader.cpp (SlangCompiler::init). If they drift, this
REM  script and the renderer can disagree about whether a shader compiles.
REM
REM  Emitted .spv files land next to their source and are gitignored (*.spv);
REM  in the in-process model the app loads .slang at runtime, so they are just
REM  validation artifacts.
REM
REM  Run from cmd / a VS Code task directly, or from git-bash with:
REM      cmd //c tools/compile_shaders.bat
REM ============================================================================
setlocal enabledelayedexpansion

REM Locate slangc via the Vulkan SDK (same env var CMake uses). Override by
REM setting SLANGC before calling, e.g. set "SLANGC=C:\path\slangc.exe".
if not defined SLANGC set "SLANGC=%VULKAN_SDK%\Bin\slangc.exe"
if not exist "%SLANGC%" (
  echo ERROR: slangc not found at "%SLANGC%". Set SLANGC or VULKAN_SDK. 1>&2
  exit /b 1
)

REM Repo root = parent of this script's folder, so cwd does not matter.
set "ROOT=%~dp0.."

set fail=0
for /r "%ROOT%\shaders" %%f in (*.slang) do (
  echo Compiling %%f
  "%SLANGC%" "%%f" -target spirv -profile spirv_1_4 -emit-spirv-directly -matrix-layout-column-major -o "%%~dpnf.spv"
  if errorlevel 1 (
    echo   FAILED: %%f
    set fail=1
  )
)

if !fail! equ 0 (
  echo All shaders compiled.
) else (
  echo One or more shaders FAILED.
)
exit /b !fail!
