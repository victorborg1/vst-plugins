#pragma once
#include <string>  
#include "Widgets/CurveView.h"

namespace oscilleon::gui {

class Lfo : public CurveView {
public:
    Lfo() = default;
    ~Lfo() = default;

    void OnMouseDown(float x, float y) override;

    void SetPlayheadPhase(float phase) { m_playheadPhase = phase; }
    void SetBeats(int beats)           { m_beats    = beats; }
    void SetSubBeats(int subBeats)     { m_subBeats = subBeats; }
    void SetBtnFont(const std::string& name) { m_btnFont = name; }

protected:
    void RenderGrid(Renderer& renderer) override;
    void RenderOverlay(Renderer& renderer) override;

private:
    bool IsInsideResetBtn(float x, float y) const;
    void GetResetBtnBounds(float& bx, float& by) const;

    float       m_playheadPhase{ -1.0f };
    int         m_beats{    4 };
    int         m_subBeats{ 4 };
    std::string m_btnFont{ "Widgets" };

    static constexpr float kBtnW = 48.0f;
    static constexpr float kBtnH = 18.0f;
};

} // namespace oscilleon::gui