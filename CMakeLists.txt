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

# ----------------------------------
# Executable
# ----------------------------------

add_executable(${NAME}
    ${SOURCES}
    ${IMGUI_SRC}
)

target_compile_definitions(${NAME} PRIVATE
    VK_NO_PROTOTYPES
    # Make the ImGui Vulkan backend resolve entrypoints through volk instead
    # of linking Vulkan prototypes (the rest of the engine uses VK_NO_PROTOTYPES).
    IMGUI_IMPL_VULKAN_USE_VOLK
    IMGUI_IMPL_VULKAN_VOLK_FILENAME=<volk/volk.h>
    ASSET_PATH="${CMAKE_SOURCE_DIR}/assets/"
    SHADER_PATH="${CMAKE_SOURCE_DIR}/shaders/"
)

target_compile_features(${NAME} PRIVATE cxx_std_20)

set_target_properties(${NAME} PROPERTIES
    DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(${NAME} PRIVATE
    source/                        # lets includes like "core/vulkanContext.h" resolve
    ${tinygltf_SOURCE_DIR}         # for tiny_gltf.h in sceneLoader.cpp
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    $ENV{VULKAN_SDK}/Include       # for volk, vma, slang headers
    ${fetch_ktx_SOURCE_DIR}/include
)

target_link_libraries(${NAME} PRIVATE
    Vulkan::Vulkan
    ktx
    ${SDL_LIBRARY}
    ${Slang_LIBRARY}
)
