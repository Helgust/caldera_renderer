add_library(scene STATIC
    sceneLoader.cpp
    sceneLoader.h
)

target_include_directories(scene PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${tinygltf_SOURCE_DIR}
    $ENV{VULKAN_SDK}/Include
)

target_link_libraries(scene PRIVATE
    tinygltf
)
