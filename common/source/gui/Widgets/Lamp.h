#pragma once
#include "Widget.h"
#include "../GuiTypes.h"
#include <glm/glm.hpp>


namespace oscilleon::gui {


class Lamp : public Widget {
public:
    Lamp() = default;
    ~Lamp() = default;

    void Render(Renderer& renderer) override;
    void Flash(LampState state);
    void Update(float dt);

    void SetState(LampState state) { m_state = state; }
    void SetOnColor(const glm::vec4& color) { m_onColor = color; }
    void SetSyncColor(const glm::vec4& color) { m_syncColor = color; }
    LampState GetState() const { return m_state; }

private:
    glm::vec4 ColorForState() const;
    float GlowForState()  const;

private:
    static constexpr float kFlashDuration = 0.05f;
    LampState m_state       { LampState::OFF };
    glm::vec4 m_onColor     { 1,1,1,1 };
    glm::vec4 m_syncColor   { 1,1,1,1 };
    glm::vec4 m_offColor    { 0,0,0,0 };
    float m_flashTimer{ 0.0f };
    float m_brightness{ 0.0f };
};


} 