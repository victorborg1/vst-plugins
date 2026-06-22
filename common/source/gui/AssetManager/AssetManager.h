#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace oscilleon::gui {

struct Character {
    unsigned int textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    FT_Pos advance;
};

struct Font {
    std::string name;
    std::unordered_map<FT_ULong, Character> characters;
    FT_Face face = nullptr;
};

class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager();

    bool Init();
    void Clean();

    std::string LoadShader(const std::string& relativePath);
    std::string GetShaderPath(const std::string& relativePath) const;

    std::string LoadTextureData(
        const std::string& filename,
        int& width, int& height, int& channels
    );
    std::string GetTexturePath(const std::string& filename) const;

    bool LoadFont(
        const std::string& filename,
        const std::string& fontName,
        int fontSize
    );
    Font* GetFont(const std::string& fontName);

private:
    bool InitFreeType();
    std::string FindResourcesPath() const;

private:
    std::string m_resourcesPath; // Plugin/Contents/Resources

    std::unordered_map<std::string, std::string> m_shaderCache;
    std::unordered_map<std::string, Font> m_fonts;

    FT_Library m_ft = nullptr;
};

} // namespace oscilleon::gui