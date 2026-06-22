#pragma once
#include <functional>
#include <string>
#include "Widget.h"
#include "../GuiTypes.h"

namespace oscilleon::gui {

    class Slider : public Widget {
    public:
        Slider() = default;
        Slider(KnobMode mode, size_t steps = 2);
        ~Slider() = default;

        void Render(Renderer& renderer) override;
        void OnMouseDown(float x, float y) override;
        void OnMouseMove(float x, float y) override;
        void OnMouseUp(float x, float y) override;

        void SetRange(float min, float max);
        void SetValue(float value);

        float GetValue() const { return m_value; }

        void SetName(const std::string& name);
        void SetStepCount(size_t steps) { m_steps = steps; }

        void SetColors(
            const glm::vec4& bg,
            const glm::vec4& fill,
            const glm::vec4& handle
        );

        void SetOnValueChanged(std::function<void(float)> callback) {
            m_callback = callback;
        }

    private:
        float NormalizeValue(float value) const;
        float DenormalizeValue(float t) const;
        float Snap(float t) const;

        float ValueFromPosition(float x) const;
        float PositionFromValue(float value) const;
        std::string FormatValue() const;

    private:
        float m_value{ 0.0f };
        float m_min{ 0.0f };
        float m_max{ 1.0f };

        float m_dragStartX{ 0.0f };
        float m_dragStartValue{ 0.0f };

        KnobMode m_mode{ KnobMode::CONTINUOUS };
        size_t m_steps{ 2 };
        bool m_dragging{ false };

        std::string m_name{ "VALUE" };

        glm::vec4 m_bgColor{ 0.15f, 0.15f, 0.15f, 1.0f };
        glm::vec4 m_fillColor{ 0.3f, 0.8f, 0.4f, 1.0f };
        glm::vec4 m_handleColor{ 0.8f, 0.8f, 0.8f, 1.0f };

        std::function<void(float)> m_callback;
    };

}