#pragma once
#include <glad/glad.h>
#include <string>
#include "RendererTypes.h"

namespace oscilleon::gui {

class Texture {
public:
    Texture();
    ~Texture();

    bool LoadFromData(const std::string& name, const unsigned char* data, int width, int height, int channels);
    bool LoadFromFile(const std::string& filepath);

    void Bind(int slot = 0) const;
    void Unbind() const;

    u32 GetID() const { return m_id; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    const std::string& GetName() const { return m_name; }

private:
    u32 m_id{ 0 };
    int m_width{ 0 };
    int m_height{ 0 };
    std::string m_name;
};

}