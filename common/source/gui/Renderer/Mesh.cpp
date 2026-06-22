#include "Mesh.h"
#include <glad/glad.h>

namespace oscilleon::gui {


Mesh::Mesh() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
}

Mesh::~Mesh() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void Mesh::UploadVertices(const std::vector<Vertex>& vertices) {
    if (vertices.empty()) return;

    m_vertexCount = static_cast<int>(vertices.size());

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::UploadVertices(const std::vector<float>& vertices) {
    if (vertices.empty()) return;

    m_vertexCount = static_cast<int>(vertices.size()) / 2;

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Mesh::Draw() const {
    if (!m_vao || m_vertexCount == 0) return;

    GLenum mode = (m_type == PrimitiveType::Lines)
        ? GL_LINES
        : GL_TRIANGLES;

    glBindVertexArray(m_vao);
    glDrawArrays(mode, 0, m_vertexCount);
    glBindVertexArray(0);
}


}