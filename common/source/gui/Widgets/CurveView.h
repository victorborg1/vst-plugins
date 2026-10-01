#pragma once
#include <functional>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "Widget.h"

namespace oscilleon::gui {

struct CurvePoint {
    float x{ 0.0f };
    float y{ 0.0f };
    bool  lockX{ false };
    bool  lockY{ false };
};

class CurveView : public Widget {
public:
    CurveView() = default;
    virtual ~CurveView() = default;

    void Render(Renderer& renderer) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseMove(float x, float y) override;
    void OnMouseUp(float x, float y) override;

    void AddPoint(float x, float y, bool lockX = false, bool lockY = false);
    void SetPoints(const std::vector<CurvePoint>& points);
    void ClearPoints();
    const std::vector<CurvePoint>& GetPoints() const { return m_points; }

    float Evaluate(float nx) const;
    void  GetSamples(int count, std::vector<float>& out) const;

    void SetCurveColor(const glm::vec4& color)     { m_curveColor = color; }
    void SetGlowIntensity(float v)                  { m_glowIntensity = v; }
    void SetLineThickness(float v)                  { m_lineThickness = v; }
    void SetLabel(const std::string& label)         { m_label = label; }

    void SetOnChanged(std::function<void(const std::vector<CurvePoint>&)> cb) {
        m_callback = cb;
    }

protected:
    virtual void RenderGrid(Renderer& renderer) {}
    virtual void RenderOverlay(Renderer& renderer) {}

    glm::vec2 ToScreen(float nx, float ny) const;
    glm::vec2 ToNormalized(float sx, float sy) const;
    glm::vec2 HandlePos(int index) const;
    void SampleCurve(int numSamples, std::vector<glm::vec2>& out) const;
    int  HitTestPoint(float sx, float sy) const;
    void SortPoints();
    bool IsInsideBounds(float x, float y) const;
    void NotifyChanged();
    void Reset();

    std::vector<CurvePoint> m_points;

    glm::vec4   m_curveColor{ 0.35f, 0.80f, 1.00f, 1.0f };
    float       m_glowIntensity{ 10.0f };
    float       m_lineThickness{ 2.5f };
    std::string m_label;

    int        m_draggingIndex{ -1 };
    bool       m_didDrag{ false };
    CurvePoint m_dragSnapshot{};

    std::function<void(const std::vector<CurvePoint>&)> m_callback;

    static constexpr int   kSamples      = 256;
    static constexpr float kPadding      = 16.0f;
    static constexpr float kHandleRadius =  6.0f;
    static constexpr float kHitRadius    = 14.0f;
};

} // namespace oscilleon::gui
