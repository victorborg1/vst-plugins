#include "Lamp.h"
namespace oscilleon::gui {

void Lamp::Flash(LampState state) {
    m_state = state;
    m_flashTimer = kFlashDuration;
    m_brightness = 1.0f;
}

void Lamp::Update(float dt)
{
    if (m_flashTimer <= 0.0f) return;
    m_flashTimer -= dt;
    if (m_flashTimer <= 0.0f) {
        m_flashTimer = 0.0f;
        m_brightness = 0.0f;
        m_state = LampState::OFF;
    }
    else {
        float t = m_flashTimer / kFlashDuration;
        m_brightness = t * t;
    }
}

glm::vec4 Lamp::ColorForState() const {
    glm::vec4 base;
    switch (m_state)
    {
    case LampState::ON:   base = m_onColor;   break;
    case LampState::SYNC: base = m_syncColor; break;
    default:              return m_offColor;
    }
    return glm::vec4(base.r, base.g, base.b, base.a * m_brightness);
}

float Lamp::GlowForState() const
{
    float peakGlow;
    switch (m_state)
    {
    case LampState::ON:   peakGlow = 10.0f;  break;
    case LampState::SYNC: peakGlow = 20.0f; break;
    default:              return 0.0f;
    }
    return peakGlow * m_brightness;
}

void Lamp::Render(Renderer& renderer)
{
    /// dark background border.
    renderer.DrawRect({ m_x, m_y }, { m_width, m_height }, { 0.08f, 0.08f, 0.08f, 1.0f });

    /// inner inset rect as the lamp face.
    constexpr float inset = 2.0f;
    glm::vec4 color = ColorForState();
    float     glow = GlowForState();

    glm::vec4 baseColor = (m_state == LampState::OFF)
        ? glm::vec4(0.06f, 0.06f, 0.06f, 1.0f)
        : color;

    renderer.DrawRect(
        { m_x + inset, m_y + inset },
        { m_width - inset * 2.0f, m_height - inset * 2.0f },
        baseColor,
        glow
    );
}


} // namespace oscilleon::gui
