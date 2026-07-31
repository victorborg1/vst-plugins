#include "Renderer.h"
#include <glad/glad.h>
#include <vector>
#include <algorithm>
#include <iostream>

namespace oscilleon::gui {

    bool Renderer::Init(int width, int height) {
        m_width = width;
        m_height = height;

        glViewport(0, 0, width, height);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_MULTISAMPLE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        if (!LoadShaders()) return false;

        m_lineMesh = std::make_unique<Mesh>();
        m_lineMesh->SetPrimitiveType(PrimitiveType::Triangles);

        m_rectMesh = std::make_unique<Mesh>();
        m_rectMesh->SetPrimitiveType(PrimitiveType::Triangles);

        m_quadMesh = std::make_unique<Mesh>();
        m_quadMesh->SetPrimitiveType(PrimitiveType::Triangles);

        m_textMesh = std::make_unique<Mesh>();
        m_textMesh->SetPrimitiveType(PrimitiveType::Triangles);

        m_projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);

        m_screenQuad = std::make_unique<Mesh>();
        m_screenQuad->SetPrimitiveType(PrimitiveType::Triangles);

        std::vector<Vertex> screenVerts = {
            {{0, 0}, {1,1,1,1}, {0,0}},
            {{1, 0}, {1,1,1,1}, {1,0}},
            {{1, 1}, {1,1,1,1}, {1,1}},
            {{0, 0}, {1,1,1,1}, {0,0}},
            {{1, 1}, {1,1,1,1}, {1,1}},
            {{0, 1}, {1,1,1,1}, {0,1}},
        };

        m_screenQuad->UploadVertices(screenVerts);

        InitFontRendering();
        InitFBOs();

        return true;
    }

    void Renderer::InitFBOs() {
        glGenFramebuffers(1, &m_hdrFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);

        glGenTextures(1, &m_colorTex);
        glBindTexture(GL_TEXTURE_2D, m_colorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

        glGenTextures(1, &m_emissiveTex);
        glBindTexture(GL_TEXTURE_2D, m_emissiveTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_emissiveTex, 0);

        glGenRenderbuffers(1, &m_depthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRBO);

        GLenum attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glGenFramebuffers(2, m_pingpongFBO);
        glGenTextures(2, m_pingpongTex);

        for (int i = 0; i < 2; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[i]);
            
            glBindTexture(GL_TEXTURE_2D, m_pingpongTex[i]);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA16F,
                m_width / 2,
                m_height / 2,
                0,
                GL_RGBA,
                GL_FLOAT,
                nullptr
            );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                m_pingpongTex[i],
                0
            );
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }

    void Renderer::ResizeFBOs() {

        glDeleteFramebuffers(1, &m_hdrFBO);
        glDeleteTextures(1, &m_colorTex);
        glDeleteTextures(1, &m_emissiveTex);
        glDeleteRenderbuffers(1, &m_depthRBO);
        glDeleteFramebuffers(2, m_pingpongFBO);
        glDeleteTextures(2, m_pingpongTex);

        InitFBOs();
    }



    bool Renderer::LoadShaders() {
        if (!m_assetManager) {
            std::cerr << "No AssetManager set!" << std::endl;
            return false;
        }

        std::string lineVS = m_assetManager->LoadShader("line.vert");
        std::string lineFS = m_assetManager->LoadShader("line.frag");
        std::string quadVS = m_assetManager->LoadShader("quad.vert");
        std::string quadFS = m_assetManager->LoadShader("quad.frag");
        std::string textVS = m_assetManager->LoadShader("text.vert");
        std::string textFS = m_assetManager->LoadShader("text.frag");
        std::string arcVS = m_assetManager->LoadShader("arc.vert");
        std::string arcFS = m_assetManager->LoadShader("arc.frag");
        std::string screenVS = m_assetManager->LoadShader("screen.vert");
        std::string screenFS = m_assetManager->LoadShader("screen.frag");
        std::string blurFS = m_assetManager->LoadShader("blur.frag");

        if (!LoadShader("line", lineVS, lineFS)) return false;
        if (!LoadShader("quad", quadVS, quadFS)) return false;
        if (!LoadShader("text", textVS, textFS)) return false;
        if (!LoadShader("arc", arcVS, arcFS)) return false;
        if (!LoadShader("screen", screenVS, screenFS)) return false;
        if (!LoadShader("blur", screenVS, blurFS))   return false;

        return true;
    }

    bool Renderer::LoadShader(const std::string& name, const std::string& vs, const std::string& fs) {
        auto shader = std::make_unique<Shader>();
        if (!shader->LoadFromSource(vs, fs)) return false;
        m_shaders[name] = std::move(shader);
        return true;
    }

    Shader* Renderer::GetShader(const std::string& name) {
        auto it = m_shaders.find(name);
        return it != m_shaders.end() ? it->second.get() : nullptr;
    }

    Texture* Renderer::LoadTexture(const std::string& name, const std::string& filename) {
        if (!m_assetManager) return nullptr;
        int width, height, channels;
        std::string textureData = m_assetManager->LoadTextureData(filename, width, height, channels);
        if (textureData.empty()) return nullptr;
        auto texture = std::make_unique<Texture>();
        if (!texture->LoadFromData(name, (const unsigned char*)textureData.data(), width, height, channels)) return nullptr;
        auto* ptr = texture.get();
        m_textures[name] = std::move(texture);
        return ptr;
    }

    Texture* Renderer::GetTexture(const std::string& name) {
        auto it = m_textures.find(name);
        return it != m_textures.end() ? it->second.get() : nullptr;
    }

    void Renderer::InitFontRendering() {
        if (!m_assetManager) return;
        for (auto& fontPair : m_fontTextures) {
            for (auto& charPair : fontPair.second) {
                glDeleteTextures(1, &charPair.second);
            }
        }
        m_fontTextures.clear();
    }

    void Renderer::Resize(int width, int height) {
        m_width = width;
        m_height = height;
        glViewport(0, 0, width, height);
        m_projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
        ResizeFBOs();
    }

/*
    void Renderer::BeginFrame(const v4& clearColor) {
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        //glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::EndFrame()
    {
        bool horizontal = true;
        bool firstPass = true;
        int blurPasses = 6;

        auto* blurShader = GetShader("blur");
        blurShader->Bind();


        glViewport(0, 0, m_width / 2, m_height / 2);
        for (int i = 0; i < blurPasses; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[horizontal]);
            blurShader->SetBool("uHorizontal", horizontal);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(
                GL_TEXTURE_2D,
                firstPass ? m_emissiveTex : m_pingpongTex[!horizontal]
            );
            blurShader->SetInt("uImage", 0);

            m_screenQuad->Draw();

            horizontal = !horizontal;
            if (firstPass) firstPass = false;
        }

        blurShader->Unbind();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_width, m_height);
        glClear(GL_COLOR_BUFFER_BIT);

        auto* screenShader = GetShader("screen");
        screenShader->Bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_colorTex);
        screenShader->SetInt("uScene", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_pingpongTex[!horizontal]);
        screenShader->SetInt("uBloom", 1);

        screenShader->SetFloat("uBloomStrength", 0.2f);

        m_screenQuad->Draw();

        screenShader->Unbind();
    }
*/

void Renderer::BeginFrame(const v4& clearColor) {
    // Store clear color for later
    m_clearColor = clearColor;
    
    // Render to HDR FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame() {
    bool horizontal = true;
    bool firstPass = true;
    int blurPasses = 6;

    auto* blurShader = GetShader("blur");
    if (!blurShader) return;
    
    blurShader->Bind();

    glViewport(0, 0, m_width / 2, m_height / 2);
    for (int i = 0; i < blurPasses; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[horizontal]);
        blurShader->SetBool("uHorizontal", horizontal);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            firstPass ? m_emissiveTex : m_pingpongTex[!horizontal]
        );
        blurShader->SetInt("uImage", 0);

        m_screenQuad->Draw();

        horizontal = !horizontal;
        if (firstPass) firstPass = false;
    }

    blurShader->Unbind();

    // NOW composite to the DEFAULT framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    auto* screenShader = GetShader("screen");
    if (!screenShader) return;
    
    screenShader->Bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    screenShader->SetInt("uScene", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_pingpongTex[!horizontal]);
    screenShader->SetInt("uBloom", 1);

    screenShader->SetFloat("uBloomStrength", 0.2f);

    m_screenQuad->Draw();

    screenShader->Unbind();
    
    // IMPORTANT: Rebind HDR FBO for the next BeginFrame
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);
}





    void Renderer::DrawMesh(const Mesh& mesh, Shader* shader) {
        DrawMeshHelper(mesh, shader, m4(1.0f));
    }

    void Renderer::DrawMeshHelper(const Mesh& mesh, Shader* shader, const m4& transform) {
        if (!shader) return;
        shader->Bind();
        shader->SetMat4("uProjection", m_projection);
        shader->SetMat4("uModel", transform);
        mesh.Draw();
        shader->Unbind();
    }

    void Renderer::DrawLine(const v2& p1, const v2& p2, const v4& color,
        float thickness, float glowIntensity)
    {
        float pad = thickness * 0.5f + glowIntensity * 4.0f + 5.0f;

        v2 minPt = glm::min(p1, p2) - v2(pad);
        v2 maxPt = glm::max(p1, p2) + v2(pad);

        std::vector<Vertex> vertices = {
            { {minPt.x, minPt.y}, color, {0,0} },
            { {maxPt.x, minPt.y}, color, {1,0} },
            { {maxPt.x, maxPt.y}, color, {1,1} },
            { {minPt.x, minPt.y}, color, {0,0} },
            { {maxPt.x, maxPt.y}, color, {1,1} },
            { {minPt.x, maxPt.y}, color, {0,1} }
        };
        m_lineMesh->UploadVertices(vertices);

        auto* shader = GetShader("line");
        shader->Bind();
        shader->SetMat4("uProjection", m_projection);
        shader->SetMat4("uModel", m4(1.0f));
        shader->SetVec2("uP1", p1);
        shader->SetVec2("uP2", p2);
        shader->SetFloat("uThickness", thickness);
        shader->SetFloat("uEmissiveStrength", glowIntensity);
        shader->Unbind();

        DrawMesh(*m_lineMesh, shader);
    }

    void Renderer::DrawRectGradient(
        const v2& position, const v2& size,
        const v4& colorTop, const v4& colorBottom,
        float glowIntensity
    ) {
        v2 min = position;
        v2 max = position + size;

        std::vector<Vertex> vertices = {
            { {min.x, min.y}, colorTop,    {0,0} },
            { {max.x, min.y}, colorTop,    {1,0} },
            { {max.x, max.y}, colorBottom, {1,1} },
            { {min.x, min.y}, colorTop,    {0,0} },
            { {max.x, max.y}, colorBottom, {1,1} },
            { {min.x, max.y}, colorBottom, {0,1} }
        };
        m_rectMesh->UploadVertices(vertices);

        auto* shader = GetShader("quad");
        shader->Bind();
        shader->SetMat4("uProjection", m_projection);
        shader->SetMat4("uModel", m4(1.0f));
        shader->SetInt("uUseTexture", 0);
        shader->SetFloat("uEmissiveStrength", glowIntensity);
        shader->Unbind();

        DrawMesh(*m_rectMesh, shader);
    }

    void Renderer::DrawRectHelper(const v2& min, const v2& max, const v4& color, float glowIntensity) {
        std::vector<Vertex> vertices = {
            { {min.x, min.y}, color, {0,0} },
            { {max.x, min.y}, color, {1,0} },
            { {max.x, max.y}, color, {1,1} },
            { {min.x, min.y}, color, {0,0} },
            { {max.x, max.y}, color, {1,1} },
            { {min.x, max.y}, color, {0,1} }
        };
        m_rectMesh->UploadVertices(vertices);

        auto* shader = GetShader("quad");
        shader->Bind();
        shader->SetMat4("uProjection", m_projection);
        shader->SetMat4("uModel", m4(1.0f));
        shader->SetInt("uUseTexture", 0);
        shader->SetFloat("uEmissiveStrength", glowIntensity);
        shader->Unbind();

        DrawMesh(*m_rectMesh, shader);
    }

    void Renderer::DrawRect(const v2& position, const v2& size, const v4& color, float glowIntensity) {
        DrawRectHelper(position, position + size, color, glowIntensity);
    }

    void Renderer::DrawQuad(const v2& p1, const v2& p2, const v2& p3, const v2& p4, const v4& color, float glowIntensity) {
        std::vector<Vertex> vertices = {
            { p1, color, {0,0} },
            { p2, color, {1,0} },
            { p3, color, {1,1} },
            { p1, color, {0,0} },
            { p3, color, {1,1} },
            { p4, color, {0,1} }
        };
        m_quadMesh->UploadVertices(vertices);

        auto* shader = GetShader("quad");
        shader->Bind();
        shader->SetMat4("uProjection", m_projection);
        shader->SetMat4("uModel", m4(1.0f));
        shader->SetInt("uUseTexture", 0);
        shader->SetFloat("uEmissiveStrength", glowIntensity);
        shader->Unbind();

        DrawMesh(*m_quadMesh, shader);
    }

    void Renderer::DrawArcHelper(
        const v2& center, float radius,
        float startAngle, float endAngle,
        const v4& colorTop, const v4& colorBottom,
        bool filled, float thickness,
        bool clockwise, float glowIntensity)
    {
        bool fullCircle = false;
        float absDelta = std::abs(endAngle - startAngle);
        if (absDelta >= glm::two_pi<float>() - 0.0001f) fullCircle = true;

        float delta = endAngle - startAngle;
        if (clockwise) { if (delta > 0.0f) delta -= glm::two_pi<float>(); }
        else { if (delta < 0.0f) delta += glm::two_pi<float>(); }
        endAngle = startAngle + delta;

        auto norm = [](float a) {
            a = std::fmod(a, glm::two_pi<float>());
            if (a < 0.0f) a += glm::two_pi<float>();
            return a;
            };
        startAngle = norm(startAngle);
        endAngle = norm(endAngle);

        float glowPadding = glowIntensity > 0.0f ? glowIntensity * 4.0f : 0.0f;
        float halfSize = radius + (filled ? 0.0f : thickness * 0.5f) + glowPadding + 5.0f * 4.0f;
        v2 min = center - v2(halfSize);
        v2 max = center + v2(halfSize);

        // use colorTop for vertex color.
        std::vector<Vertex> vertices = {
            {{min.x, min.y}, colorTop, {}},
            {{max.x, min.y}, colorTop, {}},
            {{max.x, max.y}, colorTop, {}},
            {{min.x, min.y}, colorTop, {}},
            {{max.x, max.y}, colorTop, {}},
            {{min.x, max.y}, colorTop, {}}
        };
        m_quadMesh->UploadVertices(vertices);

        auto* shader = GetShader("arc");
        shader->Bind();
        shader->SetVec2("uCenter", center);
        shader->SetFloat("uRadius", radius);
        shader->SetFloat("uThickness", thickness);
        shader->SetBool("uFilled", filled);
        shader->SetFloat("uStartAngle", startAngle);
        shader->SetFloat("uEndAngle", endAngle);
        shader->SetInt("uFullCircle", fullCircle ? 1 : 0);
        shader->SetInt("uClockwise", clockwise ? 1 : 0);
        shader->SetFloat("uEmissiveStrength", glowIntensity);
        shader->SetVec4("uColorTop", colorTop);
        shader->SetVec4("uColorBottom", colorBottom);
        DrawMesh(*m_quadMesh, shader);
    }

    void Renderer::DrawArc(
        const v2& center, float radius,
        float startAngle, float endAngle,
        const v4& color, bool filled,
        float thickness, bool clockwise, float glowIntensity
    ) {
        DrawArcHelper(center, radius, startAngle, endAngle,
                      color, color,
                      filled, thickness, clockwise, glowIntensity);
    }

    void Renderer::DrawArcGradient(
        const v2& center, float radius,
        float startAngle, float endAngle,
        const v4& colorTop, const v4& colorBottom,
        bool filled, float thickness,
        bool clockwise, float glowIntensity
    ) {
        DrawArcHelper(center, radius, startAngle, endAngle,
                      colorTop, colorBottom,
                      filled, thickness, clockwise, glowIntensity);
    }

    void Renderer::DrawTexture(Texture* texture, const v2& position, const v2& size, const v4& tint) {
        DrawTextureHelper(texture, position, size, v2(0.0f, 0.0f), v2(1.0f, 1.0f), tint);
    }

    void Renderer::DrawTextureHelper(
        Texture* texture, const v2& position, const v2& size,
        const v2& uvMin, const v2& uvMax, const v4& tint
    ) {
        if (!texture) return;

        std::vector<Vertex> vertices = {
            { {position.x, position.y}, tint, uvMin },
            { {position.x + size.x, position.y}, tint, {uvMax.x, uvMin.y} },
            { {position.x + size.x, position.y + size.y}, tint, uvMax },
            { {position.x, position.y}, tint, uvMin },
            { {position.x + size.x, position.y + size.y}, tint, uvMax },
            { {position.x, position.y + size.y}, tint, {uvMin.x, uvMax.y} }
        };
        m_quadMesh->UploadVertices(vertices);

        auto* shader = GetShader("quad");
        shader->Bind();
        shader->SetMat4("uProjection", m_projection);
        shader->SetMat4("uModel", m4(1.0f));
        shader->SetInt("uUseTexture", 1);
        shader->SetInt("uTexture", 0);
        shader->SetFloat("uEmissiveStrength", 0.0f);
        shader->Unbind();

        texture->Bind(0);
        DrawMesh(*m_quadMesh, shader);
        texture->Unbind();
    }

    void Renderer::DrawString(const std::string& text, const std::string& fontName,
        const v2& position, const v4& color, float scale) {
        if (!m_assetManager) return;
        Font* font = m_assetManager->GetFont(fontName);
        if (!font) return;

        auto* shader = GetShader("text");
        if (!shader) return;

        float x = position.x;
        float y = position.y;

        for (char c : text) {
            auto it = font->characters.find((FT_ULong)c);
            if (it == font->characters.end()) continue;
            const Character& ch = it->second;

            float xpos = x + ch.bearing.x * scale;
            float ypos = y + (ch.size.y - ch.bearing.y) * scale;
            float w = ch.size.x * scale;
            float h = ch.size.y * scale;

            std::vector<Vertex> vertices = {
                { v2(xpos,     ypos - h), color, v2(0.0f, 0.0f) },
                { v2(xpos,     ypos), color, v2(0.0f, 1.0f) },
                { v2(xpos + w, ypos), color, v2(1.0f, 1.0f) },
                { v2(xpos,     ypos - h), color, v2(0.0f, 0.0f) },
                { v2(xpos + w, ypos), color, v2(1.0f, 1.0f) },
                { v2(xpos + w, ypos - h), color, v2(1.0f, 0.0f) }
            };
            m_textMesh->UploadVertices(vertices);

            shader->Bind();
            shader->SetMat4("uProjection", m_projection);
            shader->SetInt("uTexture", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ch.textureID);
            m_textMesh->Draw();
            glBindTexture(GL_TEXTURE_2D, 0);
            shader->Unbind();

            x += (ch.advance >> 6) * scale;
        }
    }

}
