#pragma once
#include "../Renderer/Renderer.h"

namespace oscilleon::gui {

class Widget {
public:
	virtual ~Widget() = default;
	virtual void Render(Renderer& renderer) = 0;
	virtual void OnMouseDown(float x, float y) {}
	virtual void OnMouseMove(float x, float y) {}
	virtual void OnMouseUp(float x, float y) {}

	float GetX() const { return m_x; }
	float GetY() const { return m_y; }
	float GetWidth() const { return m_width; }
	float GetHeight() const { return m_height; }
	void SetPosition(float x, float y) { m_x = x; m_y = y; }
	void SetSize(float w, float h) { m_width = w; m_height = h; }


    float MeasureTextWidth(const std::string& fontName, const std::string& text, float scale, Renderer& renderer) const {
        auto* am = renderer.GetAssetManager();
        if (!am) return text.length() * 8.0f * scale;
        Font* font = am->GetFont(fontName);
        if (!font) return text.length() * 8.0f * scale;
        float w = 0.0f;
        for (char c : text) {
            auto it = font->characters.find((FT_ULong)c);
            if (it != font->characters.end())
                w += (it->second.advance >> 6) * scale;
        }
        return w;
    }

    float MeasureTextHeight(const std::string& fontName, float scale, Renderer& renderer) const {
        auto* am = renderer.GetAssetManager();
        if (!am) return 12.0f;
        Font* font = am->GetFont(fontName);
        if (!font) return 12.0f;
        auto it = font->characters.find((FT_ULong)'A');
        if (it == font->characters.end()) return 12.0f;
        return it->second.bearing.y * scale;
    }

protected:
	float m_x{ 0.0f };
	float m_y{ 0.0f };
	float m_width{ 0.0f };
	float m_height{ 0.0f };
};
	 
}


