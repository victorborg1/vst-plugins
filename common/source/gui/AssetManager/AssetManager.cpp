#include "AssetManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#pragma comment(lib, "shlwapi.lib")
#endif

#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace fs = std::filesystem;
namespace {
    void AssetManagerModuleAnchor() {}
}
namespace oscilleon::gui {

AssetManager::~AssetManager() {
    Clean();
}

bool AssetManager::Init() {
    m_resourcesPath = FindResourcesPath();

    if (m_resourcesPath.empty()) {
        std::cerr << "AssetManager: Failed to locate Resources folder\n";
        return false;
    }

    return true;
}

void AssetManager::Clean() {
    for (auto& [_, font] : m_fonts) {
        for (auto& [_, ch] : font.characters)
            glDeleteTextures(1, &ch.textureID);
        if (font.face) FT_Done_Face(font.face);
    }
    m_fonts.clear();

    if (m_ft) {
        FT_Done_FreeType(m_ft);
        m_ft = nullptr;
    }

    m_shaderCache.clear();
}

bool AssetManager::InitFreeType() {
    if (!m_ft && FT_Init_FreeType(&m_ft)) {
        std::cerr << "FreeType init failed\n";
        return false;
    }
    return true;
}

/*std::string AssetManager::FindResourcesPath() const {
#ifdef _WIN32
    HMODULE module = nullptr;

    if (!GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&AssetManagerModuleAnchor),
        &module)) {
        return {};
    }

    char path[MAX_PATH] = {};
    GetModuleFileNameA(module, path, MAX_PATH);

    std::filesystem::path p(path);

    std::filesystem::path resources =
        p.parent_path()     // x86_64-win
        .parent_path()      // Contents
        / "res";

    if (std::filesystem::exists(resources))
        return resources.string();
#endif

    return {};
}*/
std::string AssetManager::FindResourcesPath() const {
#ifdef _WIN32

    HMODULE module = nullptr;

    if (!GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&AssetManagerModuleAnchor),
        &module))
    {
        return {};
    }

    char path[MAX_PATH] = {};
    GetModuleFileNameA(module, path, MAX_PATH);

    fs::path p(path);

    // .vst3/Contents/x86_64-win/plugin.dll
    fs::path resources =
        p.parent_path()
        .parent_path()
        / "res";

    if (fs::exists(resources))
        return resources.string();


#elif defined(__linux__)

    Dl_info info{};

    if (dladdr(
        reinterpret_cast<void*>(&AssetManagerModuleAnchor),
        &info) == 0)
    {
        return {};
    }


    fs::path modulePath(info.dli_fname);

    // plugin.so:
    //
    // plug-BitDeath.vst3/
    //   Contents/
    //      x86_64-linux/
    //          plugin.so
    //
    fs::path resources =
        modulePath.parent_path()
        .parent_path()
        / "res";


    if (fs::exists(resources))
        return resources.string();

#endif


    return {};
}
std::string AssetManager::GetShaderPath(const std::string& relativePath) const {
    fs::path p = fs::path(m_resourcesPath) / "Shaders" / relativePath;
    if (fs::exists(p)) return p.string();
    return {};
}

std::string AssetManager::LoadShader(const std::string& relativePath) {
    if (auto it = m_shaderCache.find(relativePath); it != m_shaderCache.end())
        return it->second;

    std::string path = GetShaderPath(relativePath);
    if (path.empty()) return {};

    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();

    return m_shaderCache[relativePath] = ss.str();
}

std::string AssetManager::GetTexturePath(const std::string& filename) const {
    fs::path p = fs::path(m_resourcesPath) / "Textures" / filename;
    if (fs::exists(p)) return p.string();
    return {};
}

std::string AssetManager::LoadTextureData(const std::string& filename,
    int& w, int& h, int& c) {
    std::string path = GetTexturePath(filename);
    if (path.empty()) return {};

    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 0);
    if (!data) return {};

    std::string result(reinterpret_cast<char*>(data), w * h * c);
    stbi_image_free(data);
    return result;
}

bool AssetManager::LoadFont(const std::string& filename,
    const std::string& fontName,
    int fontSize) {
    if (!InitFreeType()) return false;
    if (m_fonts.find(fontName) != m_fonts.end()) return true;

    fs::path path = fs::path(m_resourcesPath) / "Fonts" / filename;
    if (!fs::exists(path)) return false;

    FT_Face face;
    if (FT_New_Face(m_ft, path.string().c_str(), 0, &face))
        return false;

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    Font font;
    font.name = fontName;
    font.face = face;

    for (FT_ULong c = 0; c < 128; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;

        unsigned int texID = 0;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0, GL_RED, GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        font.characters.emplace(c, Character{
            texID,
            { face->glyph->bitmap.width, face->glyph->bitmap.rows },
            { face->glyph->bitmap_left,  face->glyph->bitmap_top  },
            face->glyph->advance.x
            });
    }

    m_fonts.emplace(fontName, std::move(font));
    return true;
}

Font* AssetManager::GetFont(const std::string& fontName) {
    auto it = m_fonts.find(fontName);
    return it != m_fonts.end() ? &it->second : nullptr;
}

}
