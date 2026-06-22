#include "Knob.h"
#include <algorithm>
#include <cmath>
#include <cstdio>


constexpr float A_0 = 0.0f;
constexpr float A_90 = glm::half_pi<float>();
constexpr float A_180 = glm::pi<float>();
constexpr float A_270 = glm::three_over_two_pi<float>();

constexpr float A_45 = glm::quarter_pi<float>();
constexpr float A_135 = 3.0f * glm::quarter_pi<float>();
constexpr float A_225 = 5.0f * glm::quarter_pi<float>();
constexpr float A_315 = 7.0f * glm::quarter_pi<float>();


namespace oscilleon::gui {

Knob::Knob(KnobMode mode, KnobIndicatorStyle style)
    : m_mode(mode)
    , m_indicatorStyle(style)
{
}

Knob::Knob(KnobMode mode, KnobIndicatorStyle style, size_t steps)
    : m_mode(mode)
    , m_indicatorStyle(style)
    , m_steps(steps)
{
}



void Knob::SetRange(float min, float max) {
    m_min = min;
    m_max = max;
    m_value = std::clamp(m_value, m_min, m_max);
}

void Knob::SetValue(float value) {
    float t = NormalizeValue(value);
    t = Snap(t);
    m_value = DenormalizeValue(t);
    if (m_callback) m_callback(m_value);
}

void Knob::SetAngles(float startAngle, float endAngle) {
    m_startAngle = startAngle;
    m_endAngle = endAngle;
}

void Knob::SetName(const std::string& name) {
    m_name = name;
}



void Knob::UpdateValue(float normalizedValue) {
    float t = Snap(normalizedValue);
    m_value = DenormalizeValue(t);
}


float Knob::NormalizeValue(float value) const {
    return std::clamp((value - m_min) / (m_max - m_min), 0.0f, 1.0f);
}

float Knob::DenormalizeValue(float t) const {
    return m_min + t * (m_max - m_min);
}

float Knob::Snap(float t) const {
    if (m_mode != KnobMode::DISCRETE) {
        return t;
    }

    float stepsMinusOne = float(m_steps - 1);
    return std::round(t * stepsMinusOne) / stepsMinusOne;
}

float Knob::AngleFromValue(float value) const {
    constexpr float startAngle = (5.0f * glm::pi<float>()) / 4.0f;
    constexpr float endAngle = (7.0f * glm::pi<float>()) / 4.0f;

    float clockwiseDist = 360.0f - (endAngle - startAngle) * 180.0f / glm::pi<float>();
    clockwiseDist = clockwiseDist * glm::pi<float>() / 180.0f;

    float t = NormalizeValue(value);
    t = Snap(t);

    float d = t * clockwiseDist;
    float angle = startAngle - d;
    if (angle < 0.0f) angle += glm::two_pi<float>();

    return angle;
}


std::string Knob::FormatValue() const {
    char buffer[32];
    if (m_mode == KnobMode::DISCRETE) {
        snprintf(buffer, sizeof(buffer), "%.0f", m_value);
    }
    else {
        snprintf(buffer, sizeof(buffer), "%.2f", m_value);
    }
    return std::string(buffer);
}



std::vector<Indicator> Knob::BuildIndicators() const {
    std::vector<Indicator> out;

    if (m_mode == KnobMode::DISCRETE && m_steps > 1) {
        for (size_t i = 0; i < m_steps; ++i) {
            float t = float(i) / float(m_steps - 1);
            float snapped = Snap(t);
            float value = DenormalizeValue(snapped);
            float angle = AngleFromValue(value);
            out.push_back({ angle, false });
        }
        return out;
    }

    switch (m_indicatorStyle) {
    case KnobIndicatorStyle::NONE:
        break;

    case KnobIndicatorStyle::BASIC:
        out.push_back({ m_startAngle, false });
        out.push_back({ m_endAngle,   false });
        break;

    case KnobIndicatorStyle::THREE_POINT:
        out.push_back({ m_startAngle, false });
        out.push_back({ A_90,          false });
        out.push_back({ m_endAngle,    false });
        break;

    case KnobIndicatorStyle::INTERMEDIATE:
    {
        out.push_back({ m_startAngle, false });
        out.push_back({ A_90,          false });
        out.push_back({ m_endAngle,    false });

        for (float a : { 0.0f, A_45, A_135, A_180 }) {
            out.push_back({ a, true });
        }
        break;
    }

    case KnobIndicatorStyle::DISCRETE_STEPS:
        break;
    }

    return out;
}



void Knob::Render(Renderer& renderer) {
    renderer.DrawRect({ m_x, m_y }, { m_width, m_height }, { 0.1f, 0.1f, 0.1f, 1.0f });

    constexpr float kLabelScale = 0.6f;
    constexpr float kTextGap = 0.5f;
    float textHeight = MeasureTextHeight("Widgets", kLabelScale, renderer);
    float kTextReserve = textHeight + kTextGap;

    float circleAreaHeight = m_height - kTextReserve;
    float maxPossibleRadius = std::min(m_width, circleAreaHeight) * 0.5f - 2.0f;
    float circleRadius = maxPossibleRadius - 10.0f;
    float decorationOffset = 4.0f;

    glm::vec2 center(m_x + m_width * 0.5f, m_y + circleAreaHeight * 0.5f);

    renderer.DrawArc(
        center,
        circleRadius - 2.0f,
        0.0f,
        glm::two_pi<float>(),
        glm::vec4(0.06f, 0.06f, 0.06f, 1.0f),
        true, 1.0f, false, 0.0f
    );

    renderer.DrawArc(
        center,
        circleRadius - 4.0f,
        0.0f,
        glm::two_pi<float>(),
        glm::vec4(0.14f, 0.14f, 0.14f, 1.0f),
        true, 1.0f, false, 0.0f
    );

    renderer.DrawArcGradient(
        center,
        circleRadius - 11.0f,
        0.0f,
        glm::two_pi<float>(),
        glm::vec4(0.28f, 0.28f, 0.28f, 1.0f),
        glm::vec4(0.10f, 0.10f, 0.10f, 1.0f),
        1.0f, false, 0.0f
    );

    constexpr float startAngle = (5.0f * glm::pi<float>()) / 4.0f;
    float valueAngle = AngleFromValue(m_value);

    renderer.DrawArc(
        center,
        circleRadius - decorationOffset,
        startAngle,
        valueAngle,
        m_arcColor,
        false, 2.0f, true, 10.0f
    );

    auto drawLineIndicator = [&](float angle, bool dark) {
        glm::vec2 dir(std::cos(angle), -std::sin(angle));
        float outerRadius = circleRadius + 2.0f;
        float lineLength = dark ? 5.0f : 2.0f;
        glm::vec2 startPos = center + dir * outerRadius;
        glm::vec2 endPos = startPos + dir * lineLength;
        glm::vec4 color = dark
            ? glm::vec4(0.20f, 0.20f, 0.20f, 1.0f)
            : glm::vec4(0.45f, 0.45f, 0.45f, 1.0f);
        renderer.DrawLine(startPos, endPos, color, 0.5f);
        };

    for (const auto& indicator : BuildIndicators())
        drawLineIndicator(indicator.angle, indicator.dark);

    glm::vec2 needleDir(std::cos(valueAngle), -std::sin(valueAngle));
    float innerRadius = circleRadius - 11.0f;
    if (innerRadius < 0.0f) innerRadius = 0.0f;
    float indicatorLength = 7.0f;
    glm::vec2 needleStart = center + needleDir * innerRadius;
    glm::vec2 needleEnd = needleStart + needleDir * indicatorLength;
    renderer.DrawLine(needleStart, needleEnd, glm::vec4(1, 1, 1, 0.95f), 3.0f, 2.0f);

    float labelY = center.y + circleRadius + kTextGap + textHeight;

    if (m_dragging) {
        std::string valueText = FormatValue();
        float       textWidth = MeasureTextWidth("Widgets", valueText, kLabelScale, renderer);
        glm::vec2   textPos(center.x - textWidth * 0.5f, labelY);
        renderer.DrawString(valueText, "Widgets", textPos, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f), kLabelScale);
    }
    else {
        float     nameWidth = MeasureTextWidth("Widgets", m_name, kLabelScale * 0.9f, renderer);
        glm::vec2 namePos(center.x - nameWidth * 0.5f, labelY);
        renderer.DrawString(m_name, "Widgets", namePos, glm::vec4(0.55f, 0.55f, 0.55f, 0.75f), kLabelScale * 0.8f);
    }
}

void Knob::OnMouseDown(float x, float y) {
    float cx = m_x + m_width * 0.5f;
    float cy = m_y + m_height * 0.5f;
    float dx = x - cx;
    float dy = y - cy;

    float r = std::min(m_width, m_height) * 0.5f;

    if (dx * dx + dy * dy <= r * r) {
        m_dragging = true;
        m_dragStartY = y;
        m_dragStartValue = m_value;
    }
}

void Knob::OnMouseMove(float x, float y) {
    if (m_dragging) {
        float deltaY = m_dragStartY - y;
        float sensitivity = (m_max - m_min) / 200.0f;
        float newValue = m_dragStartValue + deltaY * sensitivity;
        SetValue(newValue);
    }
}

void Knob::OnMouseUp(float x, float y) {
    m_dragging = false;
}

} // namespace oscilleon::gui