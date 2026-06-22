#pragma once
#include <functional>
#include <string>
#include <glm/glm.hpp>
#include "Widget.h"
#include "../GuiTypes.h"

namespace oscilleon::gui {

class Knob : public Widget {
public:
    Knob() = default;
    Knob(KnobMode mode, KnobIndicatorStyle style);
    Knob(KnobMode mode, KnobIndicatorStyle style, size_t steps);
    ~Knob() = default;

    void Render(Renderer& renderer) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseMove(float x, float y) override;
    void OnMouseUp(float x, float y) override;

    void SetRange(float min, float max);
    void SetValue(float value);
    void UpdateValue(float normalizedValue);
    void SetName(const std::string& name);
    void SetArcColor(const glm::vec4& color) { m_arcColor = color; }
    void SetStepCount(size_t steps) { m_steps = steps; }
    void SetAngles(float startAngle, float endAngle);

    float GetStartAngle() const { return m_startAngle; }
    float GetEndAngle() const { return m_endAngle; }
    float GetValue() const { return m_value; }
    void SetOnValueChanged(std::function<void(float)> callback) { m_callback = callback; }
    bool IsDragging() const { return m_dragging; }

private:

    float AngleFromValue(float value) const;
    std::string FormatValue() const;
    float NormalizeValue(float value) const;
    float DenormalizeValue(float t) const;
    float Snap(float t) const;
    std::vector<Indicator> BuildIndicators() const;

private:

    float m_value{ 0.0f };
    float m_min{ 0.0f };
    float m_max{ 1.0f };
    std::string m_name{ "MIX" };

    float m_startAngle{ glm::pi<float>() * 5.0f / 4.0f };
    float m_endAngle{ glm::pi<float>() * 7.0f / 4.0f };
    float m_dragStartY{ 0.0f };
    float m_dragStartValue{ 0.0f };
    glm::vec4 m_arcColor{ glm::vec4{1,1,1,1} };

    KnobMode m_mode{ KnobMode::CONTINUOUS };
    KnobIndicatorStyle m_indicatorStyle{KnobIndicatorStyle::THREE_POINT};
    size_t m_steps{ 2 };
    bool m_dragging{ false };
    std::function<void(float)> m_callback;
};

}