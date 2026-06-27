cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(NAME CalderaRenderer)

project(${NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Glob all .cpp files under source/ recursively
# Picks up core/, resources/, rendering/, scene/, and main.cpp automatically
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "source/*.cpp")

include(FetchContent)

# ----------------------------------
# Vulkan
# ----------------------------------

find_package(Vulkan REQUIRED)

# ----------------------------------
# tinygltf
# ----------------------------------

FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v3.0.0
)
FetchContent_MakeAvailable(tinygltf)

# ----------------------------------
# ImGui
# ----------------------------------

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8-docking
)
FetchContent_MakeAvailable(imgui)

# ----------------------------------
# KTX (minimal build)
# ----------------------------------

set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_LOADTEST_APPS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_WRITE OFF CACHE BOOL "" FORCE)
message(NOTICE "Fetching LibKTX from https://github.com/KhronosGroup/KTX-Software ...")
set(KTX_FEATURE_STATIC_LIBRARY ON CACHE BOOL "Build KTX as a static library" FORCE)
FetchContent_Declare(
    fetch_ktx
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG v4.4.2
)
FetchContent_MakeAvailable(fetch_ktx)

# ----------------------------------
# minilog (thread-safe logging)
# ----------------------------------

set(MINILOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    minilog
    GIT_REPOSITORY https://github.com/corporateshark/minilog.git
    GIT_TAG 066506bfc2f9f57ee96035fcdd1b67262a8746b0
)
# Force a static lib regardless of BUILD_SHARED_LIBS: minilog's sources carry no
# dllexport markup, so a DLL would export nothing and fail to link against it.
set(_caldera_prev_shared ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(minilog)
set(BUILD_SHARED_LIBS ${_caldera_prev_shared})
# minilog's CMakeLists adds no usage requirements, so publish its header dir on
# the target. MINILOG_ENABLE_VA_LIST exposes the logVAList() entry points the
# caldera logging facade (source/core/log.cpp) forwards into.
target_include_directories(minilog PUBLIC ${minilog_SOURCE_DIR})
target_compile_definitions(minilog PUBLIC MINILOG_ENABLE_VA_LIST)

find_library(SDL_LIBRARY NAMES SDL3 HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)
find_library(Slang_LIBRARY NAMES slang slang-compiler HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")

# ----------------------------------
# ImGui sources
# ----------------------------------

set(IMGUI_SRC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

# Build ImGui as its own target so engine-only warning flags (-Wshadow-all)
# don't fire on third-party sources we don't own.
add_library(imgui_backend STATIC ${IMGUI_SRC})

# PUBLIC: our code (ui/imguiLayer.cpp, main.cpp) includes these headers, so the
# include dirs + volk-routing defines must propagate to the exe. The backend
# header compiles to different signatures with vs. without USE_VOLK, so the
# define has to be identical on both sides of the link boundary.
target_include_directories(imgui_backend PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_definitions(imgui_backend PUBLIC
    VK_NO_PROTOTYPES
    IMGUI_IMPL_VULKAN_USE_VOLK
    IMGUI_IMPL_VULKAN_VOLK_FILENAME=<volk/volk.h>
)

# PRIVATE: only the backend .cpp need volk/SDL3 headers to compile; the exe
# already carries this include for its own volk/vma/slang use. SDL is an
# implementation dependency of the SDL3 backend.
target_include_directories(imgui_backend PRIVATE
    $ENV{VULKAN_SDK}/Include
)
target_link_libraries(imgui_backend PRIVATE ${SDL_LIBRARY})

# ----------------------------------
# Executable
# ----------------------------------

add_executable(${NAME}
    ${SOURCES}
)
target_compile_options(${NAME} PRIVATE -Wshadow-all)

target_compile_definitions(${NAME} PRIVATE
    VK_NO_PROTOTYPES
    ASSET_PATH="${CMAKE_SOURCE_DIR}/assets/"
    SHADER_PATH="${CMAKE_SOURCE_DIR}/shaders/"
    CALDERA_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

target_compile_features(${NAME} PRIVATE cxx_std_20)

set_target_properties(${NAME} PROPERTIES
    DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(${NAME} PRIVATE
    source/                        # lets includes like "core/vulkanContext.h" resolve
    ${tinygltf_SOURCE_DIR}         # for tiny_gltf.h in sceneLoader.cpp
    $ENV{VULKAN_SDK}/Include       # for volk, vma, slang headers
    ${fetch_ktx_SOURCE_DIR}/include
)
# imgui include dirs come in transitively as PUBLIC usage requirements of
# imgui_backend, so they're no longer listed here.

target_link_libraries(${NAME} PRIVATE
    Vulkan::Vulkan
    ktx
    minilog
    imgui_backend
    ${SDL_LIBRARY}
    ${Slang_LIBRARY}
)
