#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "../AssetManager/AssetManager.h"
#include "RendererTypes.h"

namespace oscilleon::gui {


class Renderer {
public:
    bool Init(int width, int height);
    void Resize(int width, int height);
    void BeginFrame(const v4& clearColor);
    void EndFrame();
    void SetAssetManager(AssetManager* assetManager) { m_assetManager = assetManager; }
    AssetManager* GetAssetManager() { return m_assetManager; }

    bool LoadShader(const std::string& name, const std::string& vs, const std::string& fs);
    Shader* GetShader(const std::string& name);
    Texture* LoadTexture(const std::string& name, const std::string& filename);
    Texture* GetTexture(const std::string& name);


    void DrawMesh(const Mesh& mesh, Shader* shader);
    void DrawLine(const v2& p1, const v2& p2, const v4& color, float thickness = 1.0f, float glowIntensity = 0.0f);
    void DrawRect(const v2& position, const v2& size, const v4& color, float glowIntensity = 0.0f);
    void DrawRectGradient(const v2& position, const v2& size,
                          const v4& colorTop, const v4& colorBottom,
                          float glowIntensity = 0.0f);
    void DrawQuad(const v2& p1, const v2& p2, const v2& p3, const v2& p4, const v4& color, float glowIntensity = 0.0f);
    void DrawTexture(Texture* texture, const v2& position, const v2& size, const v4& tint);
    void DrawString(const std::string& text, const std::string& fontName, const v2& position, const v4& color, float scale);


    void DrawArc(
        const v2& center,
        float radius,
        float startAngle,
        float endAngle,
        const v4& color,
        bool filled,
        float thickness,
        bool clockwise,
        float glowIntensity = 0.0f
    );

    void DrawArcGradient(
        const v2& center,
        float radius,
        float startAngle,
        float endAngle,
        const v4& colorTop,
        const v4& colorBottom,
        bool filled,
        float thickness,
        bool clockwise,
        float glowIntensity = 0.0f
    );


private:

    void DrawArcHelper(
        const v2& center, float radius,
        float startAngle, float endAngle,
        const v4& colorTop, const v4& colorBottom,
        bool filled, float thickness,
        bool clockwise, float glowIntensity
    );
    void DrawRectHelper(const v2& min, const v2& max, const v4& color, float glowIntensity = 0.0f);
    void DrawTextureHelper(
        Texture* texture, const v2& position, const v2& size,
        const v2& uvMin, const v2& uvMax, const v4& tint
    );
    void DrawMeshHelper(const Mesh& mesh, Shader* shader, const m4& transform);
    void InitFontRendering();
    bool LoadShaders();
    void InitFBOs();
    void ResizeFBOs();


private:
    int m_width{ 0 };
    int m_height{ 0 };
    m4 m_projection{ 1.0f };

    AssetManager* m_assetManager{ nullptr };

    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;

    std::unique_ptr<Mesh> m_lineMesh;
    std::unique_ptr<Mesh> m_rectMesh;
    std::unique_ptr<Mesh> m_quadMesh;
    std::unique_ptr<Mesh> m_textMesh;


    bool m_batching{ false };
    bool m_inTextBatch{ false };

    std::unordered_map<std::string, std::unordered_map<FT_ULong, u32>> m_fontTextures;

private:
    /*gaussian bloom*/
    u32 m_hdrFBO;
    u32 m_colorTex;    // scene color
    u32 m_emissiveTex; // bloom mask
    u32 m_depthRBO;
    std::unique_ptr<Mesh> m_screenQuad;
    u32 m_pingpongFBO[2];
    u32 m_pingpongTex[2];


};

}