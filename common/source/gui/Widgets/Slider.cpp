#include "Slider.h"
#include <algorithm>
#include <cstdio>

namespace oscilleon::gui {

Slider::Slider(KnobMode mode, size_t steps)
    : m_mode(mode)
    , m_steps(steps)
{
    m_height = 22.0f;
}

void Slider::SetRange(float min, float max) {
    m_min = min;
    m_max = max;
    m_value = std::clamp(m_value, m_min, m_max);
}

void Slider::SetValue(float value) {
    float t = NormalizeValue(value);
    t = Snap(t);
    m_value = DenormalizeValue(t);
    if (m_callback) m_callback(m_value);
}

void Slider::SetName(const std::string& name) {
    m_name = name;
}

void Slider::SetColors(
    const glm::vec4& bg,
    const glm::vec4& fill,
    const glm::vec4& handle
) {
    m_bgColor = bg;
    m_fillColor = fill;
    m_handleColor = handle;
}

float Slider::NormalizeValue(float value) const {
    return std::clamp((value - m_min) / (m_max - m_min), 0.0f, 1.0f);
}

float Slider::DenormalizeValue(float t) const {
    return m_min + t * (m_max - m_min);
}

float Slider::Snap(float t) const {
    if (m_mode != KnobMode::DISCRETE || m_steps < 2)
        return t;

    float stepsMinusOne = float(m_steps - 1);
    return std::round(t * stepsMinusOne) / stepsMinusOne;
}

float Slider::ValueFromPosition(float x) const {
    float t = (x - m_x) / m_width;
    t = std::clamp(t, 0.0f, 1.0f);
    t = Snap(t);
    return DenormalizeValue(t);
}

float Slider::PositionFromValue(float value) const {
    float t = NormalizeValue(value);
    t = Snap(t);
    return m_x + t * m_width;
}

std::string Slider::FormatValue() const {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", m_value);
    return buf;
}

void Slider::Render(Renderer& renderer) {
    renderer.DrawRect({ m_x, m_y }, { m_width, m_height }, { 0.15f, 0.15f, 0.15f, 1.0f });
    renderer.DrawRect({ m_x + 4.0f, m_y + 4.0f }, { m_width - 4.0f, m_height - 4.0f }, { 0.2f, 0.2f, 0.2f, 1.0f });

    float handleX = PositionFromValue(m_value);

    renderer.DrawRect(
        { m_x, m_y - (m_height * 0.1f - m_height) * 0.5f },
        { handleX - m_x, m_height * 0.1f },
        m_fillColor
    );

    float handleSize = m_height * 0.5f;
    float hx = handleX - handleSize * 0.5f;
    float hy = m_y - (handleSize - m_height) * 0.5f;

    renderer.DrawRect(
        { hx, hy },
        { handleSize, handleSize },
        m_handleColor
    );

    renderer.DrawRect(
        { hx, hy },
        { handleSize, handleSize },
        { 0, 0, 0, 0.3f }
    );

    //if (m_dragging) {
    //    renderer.DrawString(
    //        FormatValue(),
    //        "Widgets",
    //        { m_x + m_width * 0.5f - 12.0f, m_y - 18.0f },
    //        { 1, 1, 1, 1 },
    //        1.0f
    //    );
    //}
}

void Slider::OnMouseDown(float x, float y) {
    if (x >= m_x && x <= m_x + m_width &&
        y >= m_y && y <= m_y + m_height) {
        m_dragging = true;
        m_dragStartX = x;
        m_dragStartValue = m_value;
        SetValue(ValueFromPosition(x));
    }
}

void Slider::OnMouseMove(float x, float y) {
    if (!m_dragging) return;

    float dx = x - m_dragStartX;
    float sensitivity = (m_max - m_min) / m_width;
    SetValue(m_dragStartValue + dx * sensitivity);
}

void Slider::OnMouseUp(float, float) {
    m_dragging = false;
}

}