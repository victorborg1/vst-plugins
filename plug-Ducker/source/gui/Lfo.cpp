#include "Lfo.h"
#include <glm/gtc/constants.hpp>

namespace oscilleon::gui {

void Lfo::GetResetBtnBounds(float& bx, float& by) const {
    bx = m_x + m_width  - kPadding - kBtnW;
    by = m_y + 4.0f;
}

bool Lfo::IsInsideResetBtn(float x, float y) const {
    float bx, by;
    GetResetBtnBounds(bx, by);
    return x >= bx && x <= bx + kBtnW &&
           y >= by && y <= by + kBtnH;
}

void Lfo::RenderGrid(Renderer& renderer) {
    int total = m_beats * m_subBeats;

    for (int b = 0; b <= total; ++b) {
        float t    = float(b) / float(total);
        float gx   = m_x + kPadding + t * (m_width - 2.0f * kPadding);
        bool  beat = (b % m_subBeats == 0);
        renderer.DrawLine({ gx, m_y + kPadding },
                          { gx, m_y + m_height - kPadding },
                          { 0.28f, 0.10f, 0.10f, beat ? 0.55f : 0.25f },
                          beat ? 1.0f : 0.5f, 0.0f);
    }

    for (int h = 0; h <= 4; ++h) {
        float t  = float(h) / 4.0f;
        float gy = m_y + kPadding + t * (m_height - 2.0f * kPadding);
        renderer.DrawLine({ m_x + kPadding, gy },
                          { m_x + m_width - kPadding, gy },
                          { 0.28f, 0.10f, 0.10f, h == 2 ? 0.45f : 0.20f },
                          0.5f, 0.0f);
    }
}

void Lfo::RenderOverlay(Renderer& renderer) {
    if (m_playheadPhase >= 0.0f) {
        float phx = m_x + kPadding + m_playheadPhase * (m_width - 2.0f * kPadding);

        renderer.DrawRectGradient(
            { phx - 10.0f, m_y + kPadding },
            { 20.0f, m_height - 2.0f * kPadding },
            { 1.0f, 0.3f, 0.3f, 0.06f },
            { 1.0f, 0.3f, 0.3f, 0.00f });

        
        renderer.DrawLine({ phx, m_y + kPadding },
                          { phx, m_y + m_height - kPadding },
                          { 0.4f, 0.6f, 1.0f, 0.80f }, 1.5f, 10.0f);

        glm::vec2 dot = ToScreen(m_playheadPhase, Evaluate(m_playheadPhase));
        renderer.DrawArc(dot, 4.5f, 0.0f, glm::two_pi<float>(),
                         { 0.4f, 0.6f, 1.0f, 1.0f }, true, 1.0f, false, 0.0f);
        renderer.DrawArc(dot, 4.5f, 0.0f, glm::two_pi<float>(),
                         m_curveColor, false, 1.5f, false, 10.0f);
    }

    float bx, by;
    GetResetBtnBounds(bx, by);

    renderer.DrawRect({ bx, by }, { kBtnW, kBtnH },
                      { 0.12f, 0.04f, 0.04f, 1.0f });
    renderer.DrawLine({ bx,         by         }, { bx + kBtnW, by         },
                      { 0.30f, 0.10f, 0.10f, 0.4f }, 1.0f, 0.0f);
    renderer.DrawLine({ bx,         by + kBtnH }, { bx + kBtnW, by + kBtnH },
                      { 0.30f, 0.10f, 0.10f, 0.4f }, 1.0f, 0.0f);
    renderer.DrawLine({ bx,         by         }, { bx,         by + kBtnH },
                      { 0.30f, 0.10f, 0.10f, 0.4f }, 1.0f, 0.0f);
    renderer.DrawLine({ bx + kBtnW, by         }, { bx + kBtnW, by + kBtnH },
                      { 0.30f, 0.10f, 0.10f, 0.4f }, 1.0f, 0.0f);

    constexpr float kTextPaddingV = 4.0f;
    float textHeightAt1 = MeasureTextHeight(m_btnFont, 1.0f, renderer);
    float textScale = (kBtnH - kTextPaddingV * 2.0f) / textHeightAt1;
    float tw = MeasureTextWidth(m_btnFont, "RESET", textScale, renderer);
    float th = textHeightAt1 * textScale;

    renderer.DrawString("RESET", m_btnFont,
        { bx + (kBtnW - tw) * 0.5f, by + (kBtnH + th) * 0.5f },
        { 0.50f, 0.22f, 0.22f, 0.9f }, textScale);
}


void Lfo::OnMouseDown(float x, float y) {
    if (IsInsideResetBtn(x, y)) {
        Reset();
        return;
    }
    CurveView::OnMouseDown(x, y);
}

} // namespace oscilleon::gui