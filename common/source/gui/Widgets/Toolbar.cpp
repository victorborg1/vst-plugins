#include "Toolbar.h"
#include "../Renderer/Renderer.h"

namespace oscilleon::gui {


void Toolbar::Render(Renderer& renderer) {
    renderer.DrawRect({ m_x, m_y }, { m_width,m_height }, { 0.1f,0.1f,0.1f,1.0f });

    constexpr float textScale = 0.5f * 0.6f;
    const glm::vec4 textColor{ 0.5f, 0.5f, 0.5f, 1.0f };
    constexpr float margin = 20.0f;

    float textHeight = MeasureTextHeight("Labels", textScale, renderer);

    float centerY = m_y + m_height * 0.5f;
    float baselineY = centerY + textHeight * 0.5f;

    renderer.DrawString(
        m_company,
        "Labels",
        { m_x + margin, baselineY },
        textColor,
        textScale
    );


    float pluginWidth =
        MeasureTextWidth("Labels", m_plugin, textScale, renderer);

    renderer.DrawString(
        m_plugin,
        "Labels",
        { m_x + m_width - pluginWidth - margin, baselineY },
        textColor,
        textScale
    );
}

} // namespace oscilleon::gui