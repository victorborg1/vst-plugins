#include "Texture.h"
#include <iostream>
#include "stb_image.h"

namespace oscilleon::gui {

Texture::Texture() {
    glGenTextures(1, &m_id);
}

Texture::~Texture() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
    }
}

bool Texture::LoadFromData(const std::string& name, const unsigned char* data, int width, int height, int channels) {
    m_name = name;
    m_width = width;
    m_height = height;

    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Created texture: " << name << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
    return true;
}

bool Texture::LoadFromFile(const std::string& filepath) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

    if (!data) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return false;
    }

    bool result = LoadFromData(filepath, data, width, height, channels);
    stbi_image_free(data);

    return result;
}

void Texture::Bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

}