#include "scene/sceneLoader.h"
#include <iostream>
#include <unordered_map>
#include "core/assert.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace caldera {

Scene SceneLoader::load(const std::string& path) {
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  const bool loaded = loader.LoadASCIIFromFile(&model, &err, &warn, path);

  if (!warn.empty())
    std::cout << "glTF warning: " << warn << "\n";
  if (!err.empty())
    std::cout << "glTF error: " << err << "\n";
  CALDERA_ASSERT_MSG(loaded, "Failed to load glTF model '%s': %s", path.c_str(),
                     err.empty() ? "(no detail)" : err.c_str());

  Scene scene;
  std::unordered_map<Vertex, uint16_t> uniqueVertices;

  for (const auto& mesh : model.meshes) {
    for (const auto& primitive : mesh.primitives) {
      const tinygltf::Accessor& indexAccessor =
        model.accessors[primitive.indices];
      const tinygltf::BufferView& indexBufferView =
        model.bufferViews[indexAccessor.bufferView];
      const tinygltf::Buffer& indexBuffer =
        model.buffers[indexBufferView.buffer];

      const tinygltf::Accessor& posAccessor =
        model.accessors[primitive.attributes.at("POSITION")];
      const tinygltf::BufferView& posBufferView =
        model.bufferViews[posAccessor.bufferView];
      const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];

      bool hasTexCoords =
        primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end();
      const tinygltf::Accessor* texCoordAccessor = nullptr;
      const tinygltf::BufferView* texCoordBufferView = nullptr;
      const tinygltf::Buffer* texCoordBuffer = nullptr;
      if (hasTexCoords) {
        texCoordAccessor =
          &model.accessors[primitive.attributes.at("TEXCOORD_0")];
        texCoordBufferView = &model.bufferViews[texCoordAccessor->bufferView];
        texCoordBuffer = &model.buffers[texCoordBufferView->buffer];
      }

      bool hasNormals =
        primitive.attributes.find("NORMAL") != primitive.attributes.end();
      const tinygltf::Accessor* normalAccessor = nullptr;
      const tinygltf::BufferView* normalBufferView = nullptr;
      const tinygltf::Buffer* normalBuffer = nullptr;
      if (hasNormals) {
        normalAccessor = &model.accessors[primitive.attributes.at("NORMAL")];
        normalBufferView = &model.bufferViews[normalAccessor->bufferView];
        normalBuffer = &model.buffers[normalBufferView->buffer];
      }

      const unsigned char* indexData =
        &indexBuffer
           .data[indexBufferView.byteOffset + indexAccessor.byteOffset];

      for (size_t i = 0; i < indexAccessor.count; i++) {
        uint16_t index = 0;
        if (indexAccessor.componentType ==
            TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
          index = reinterpret_cast<const uint16_t*>(indexData)[i];
        } else if (indexAccessor.componentType ==
                   TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
          index = static_cast<uint16_t>(
            reinterpret_cast<const uint32_t*>(indexData)[i]);
        } else if (indexAccessor.componentType ==
                   TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
          index = reinterpret_cast<const uint8_t*>(indexData)[i];
        }

        Vertex vertex{};

        size_t posStride = posAccessor.ByteStride(posBufferView);
        if (posStride == 0)
          posStride = sizeof(float) * 3;
        const float* pos = reinterpret_cast<const float*>(
          &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset +
                          index * posStride]);
        vertex.pos = {pos[0], pos[1], pos[2]};

        if (hasNormals) {
          size_t normalStride = normalAccessor->ByteStride(*normalBufferView);
          if (normalStride == 0)
            normalStride = sizeof(float) * 3;
          const float* normal = reinterpret_cast<const float*>(
            &normalBuffer
               ->data[normalBufferView->byteOffset +
                      normalAccessor->byteOffset + index * normalStride]);
          vertex.normal = {normal[0], normal[1], normal[2]};
        } else {
          vertex.normal = {0.0f, 0.0f, 1.0f};
        }

        if (hasTexCoords) {
          size_t uvStride = texCoordAccessor->ByteStride(*texCoordBufferView);
          if (uvStride == 0)
            uvStride = sizeof(float) * 2;
          const float* uv = reinterpret_cast<const float*>(
            &texCoordBuffer
               ->data[texCoordBufferView->byteOffset +
                      texCoordAccessor->byteOffset + index * uvStride]);
          vertex.uv = {uv[0], uv[1]};
        } else {
          vertex.uv = {0.0f, 0.0f};
        }

        vertex.color = {1.0f, 1.0f, 1.0f};

        if (!uniqueVertices.contains(vertex)) {
          uniqueVertices[vertex] =
            static_cast<uint16_t>(scene.mesh.vertices.size());
          scene.mesh.vertices.push_back(vertex);
        }
        scene.mesh.indices.push_back(uniqueVertices[vertex]);
      }
    }
  }

  scene.images.reserve(model.images.size());
  for (const auto& image : model.images) {
    scene.images.push_back(ImageData{
      .width = image.width, .height = image.height, .pixels = image.image});
  }

  return scene;
}

}  // namespace caldera
