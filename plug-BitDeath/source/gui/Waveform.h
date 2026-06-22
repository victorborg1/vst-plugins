#pragma once
#include "Widgets/Widget.h"
#include <glm/glm.hpp>

namespace oscilleon::gui {

class Waveform : public Widget {
public:
    Waveform() = default;
    ~Waveform() = default;

    void Render(Renderer& renderer) override;
    void OnMouseDown(float, float) override {}
    void OnMouseMove(float, float) override {}
    void OnMouseUp  (float, float) override {}

    void SetParams(float downsampleSteps, float bitDepth, float mix);

private:
    float quantize(float x, int bits) const;

    float m_downsampleSteps{ 1.0f };
    float m_bitDepth{ 24.0f };
    float m_mix{ 1.0f };
};

} // namespace oscilleon::gui