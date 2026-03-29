#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace caldera {

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
  glm::vec3 color;

  bool operator==(const Vertex& other) const {
    return glm::all(glm::epsilonEqual(pos, other.pos, 0.0001f)) &&
           glm::all(glm::epsilonEqual(normal, other.normal, 0.0001f)) &&
           glm::all(glm::epsilonEqual(uv, other.uv, 0.0001f)) &&
           glm::all(glm::epsilonEqual(color, other.color, 0.0001f));
  }
};

}  // namespace caldera

// Hash must live in std:: — specialise on the fully-qualified type
namespace std {
template <>
struct hash<caldera::Vertex> {
  size_t operator()(caldera::Vertex const& v) const {
    return ((hash<glm::vec3>()(v.pos) ^ (hash<glm::vec3>()(v.normal) << 1)) >>
            1) ^
           (hash<glm::vec3>()(v.color) << 1) ^ (hash<glm::vec2>()(v.uv) << 1);
  }
};
}  // namespace std

namespace caldera {

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;
};

struct ImageData {
  int width{0};
  int height{0};
  std::vector<unsigned char> pixels;
};

struct Scene {
  Mesh mesh;
  std::vector<ImageData> images;
};

class SceneLoader {
 public:
  Scene load(const std::string& path);
};

}  // namespace caldera
