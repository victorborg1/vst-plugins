#pragma once
#include <glm/glm.hpp>

namespace oscilleon::gui {

using u32 = unsigned int;
using v2 = glm::vec2;
using v3 = glm::vec3;
using v4 = glm::vec4;
using m4 = glm::mat4;

struct Glyph {
    v2 position;
    v2 size;
    v2 texCoordMin;
    v2 texCoordMax;
    u32 textureID;
    v4 color;
};

enum class PrimitiveType {
    Lines,
    Triangles
};

struct Vertex {
    v2 position;
    v4 color;
    v2 texCoord;
};



}