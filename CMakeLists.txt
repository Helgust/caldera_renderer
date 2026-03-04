cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(NAME CalderaRenderer)

project(${NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    GIT_TAG release
)

FetchContent_MakeAvailable(tinygltf)

# ----------------------------------
# ImGui
# ----------------------------------

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
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

FetchContent_Declare(
    ktx
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG main
)

FetchContent_MakeAvailable(ktx)

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
)

# ----------------------------------
# Executable
# ----------------------------------

add_executable(${NAME}
    source/main.cpp
    ${IMGUI_SRC}
)

target_compile_definitions(${NAME} PRIVATE VK_NO_PROTOTYPES)
target_compile_features(${NAME} PRIVATE cxx_std_20)
set_target_properties(${NAME} PROPERTIES DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})


target_include_directories(${NAME} PRIVATE
    ${tinygltf_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
)

target_link_libraries(${NAME} PRIVATE
    Vulkan::Vulkan
    ktx
    ${SDL_LIBRARY}
    ${Slang_LIBRARY}
)