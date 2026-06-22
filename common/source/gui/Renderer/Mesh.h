#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "RendererTypes.h"

namespace oscilleon::gui {


class Mesh {
public:
    Mesh();
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void UploadVertices(const std::vector<Vertex>& vertices);
    void UploadVertices(const std::vector<float>& vertices);

    void SetPrimitiveType(PrimitiveType type) { m_type = type; }
    void Draw() const;

private:
    u32 m_vao{ 0 };
    u32 m_vbo{ 0 };
    int m_vertexCount{ 0 };
    PrimitiveType m_type{ PrimitiveType::Triangles };
};


}