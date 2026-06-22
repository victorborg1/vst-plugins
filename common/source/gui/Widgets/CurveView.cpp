#include "CurveView.h"
#include <algorithm>
#include <cmath>

namespace oscilleon::gui {

void CurveView::SortPoints() {
    std::sort(m_points.begin(), m_points.end(),
              [](const CurvePoint& a, const CurvePoint& b){ return a.x < b.x; });
}

bool CurveView::IsInsideBounds(float x, float y) const {
    return x >= m_x && x <= m_x + m_width &&
           y >= m_y && y <= m_y + m_height;
}

glm::vec2 CurveView::ToScreen(float nx, float ny) const {
    float iw = m_width  - 2.0f * kPadding;
    float ih = m_height - 2.0f * kPadding;
    return { m_x + kPadding + nx          * iw,
             m_y + kPadding + (1.0f - ny) * ih };
}

glm::vec2 CurveView::ToNormalized(float sx, float sy) const {
    float iw = m_width  - 2.0f * kPadding;
    float ih = m_height - 2.0f * kPadding;
    return { std::clamp((sx - m_x - kPadding) / iw, 0.0f, 1.0f),
             std::clamp(1.0f - (sy - m_y - kPadding) / ih, 0.0f, 1.0f) };
}

glm::vec2 CurveView::HandlePos(int index) const {
    const CurvePoint& p = m_points[index];
    return ToScreen(p.x, p.y);
}

void CurveView::AddPoint(float x, float y, bool lockX, bool lockY) {
    m_points.push_back({ x, y, lockX, lockY });
    SortPoints();
}

void CurveView::SetPoints(const std::vector<CurvePoint>& points) {
    m_points = points;
    SortPoints();
}

void CurveView::ClearPoints() { m_points.clear(); }

void CurveView::NotifyChanged() {
    if (m_callback) m_callback(m_points);
}

void CurveView::Reset() {
    m_points.erase(
        std::remove_if(m_points.begin(), m_points.end(),
                       [](const CurvePoint& p){ return !p.lockX; }),
        m_points.end());
    for (auto& p : m_points) p.y = 0.0f;
    SortPoints();
    NotifyChanged();
}

float CurveView::Evaluate(float nx) const {
    nx = std::clamp(nx, 0.0f, 1.0f);
    int n = static_cast<int>(m_points.size());
    if (n == 0) return 0.0f;
    if (n == 1) return m_points[0].y;
    if (nx <= m_points[0].x)   return m_points[0].y;
    if (nx >= m_points[n-1].x) return m_points[n-1].y;
    for (int i = 0; i < n - 1; ++i) {
        if (nx <= m_points[i + 1].x) {
            float dx = m_points[i+1].x - m_points[i].x;
            if (dx < 1e-6f) return m_points[i].y;
            float t = (nx - m_points[i].x) / dx;
            return m_points[i].y + t * (m_points[i+1].y - m_points[i].y);
        }
    }
    return m_points[n-1].y;
}

void CurveView::GetSamples(int count, std::vector<float>& out) const {
    out.resize(count);
    for (int i = 0; i < count; ++i)
        out[i] = Evaluate(float(i) / float(std::max(count - 1, 1)));
}

void CurveView::SampleCurve(int numSamples, std::vector<glm::vec2>& out) const {
    out.clear();
    out.reserve(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        float nx = float(i) / float(numSamples - 1);
        out.push_back(ToScreen(nx, Evaluate(nx)));
    }
}

int CurveView::HitTestPoint(float sx, float sy) const {
    for (int i = 0; i < static_cast<int>(m_points.size()); ++i) {
        glm::vec2 pos = HandlePos(i);
        float dx = sx - pos.x, dy = sy - pos.y;
        if (dx*dx + dy*dy <= kHitRadius * kHitRadius) return i;
    }
    return -1;
}

void CurveView::Render(Renderer& renderer) {
    renderer.DrawRect({ m_x, m_y }, { m_width, m_height },
                      { 0.06f, 0.02f, 0.02f, 1.0f });
    renderer.DrawRectGradient(
        { m_x + 2.0f, m_y + 2.0f }, { m_width - 4.0f, m_height - 4.0f },
        { 0.10f, 0.04f, 0.04f, 1.0f },
        { 0.06f, 0.02f, 0.02f, 1.0f });

    RenderGrid(renderer);

    std::vector<glm::vec2> samples;
    SampleCurve(kSamples, samples);
    const float bottom = m_y + m_height - kPadding;

    for (int i = 0; i + 1 < static_cast<int>(samples.size()); ++i) {
        float stripW = samples[i + 1].x - samples[i].x;
        float fillH  = bottom - samples[i].y;
        if (stripW < 0.5f || fillH <= 0.0f) continue;
        renderer.DrawRectGradient(
            { samples[i].x, samples[i].y }, { stripW, fillH },
            { m_curveColor.r, m_curveColor.g, m_curveColor.b, 0.22f },
            { m_curveColor.r, m_curveColor.g, m_curveColor.b, 0.02f });
    }

    for (int i = 0; i + 1 < static_cast<int>(samples.size()); ++i)
        renderer.DrawLine(samples[i], samples[i + 1],
                          m_curveColor, m_lineThickness, m_glowIntensity);

    for (int i = 0; i < static_cast<int>(m_points.size()); ++i) {
        glm::vec2 pos    = HandlePos(i);
        bool      active = (i == m_draggingIndex);
        renderer.DrawArc(pos, kHandleRadius + 4.0f, 0.0f, glm::two_pi<float>(),
                         { m_curveColor.r, m_curveColor.g, m_curveColor.b, active ? 0.28f : 0.08f },
                         true, 1.0f, false, 0.0f);
        glm::vec4 fill = active
            ? glm::vec4(m_curveColor.r * 0.6f, 0.06f, 0.06f, 1.0f)
            : glm::vec4(0.14f, 0.05f, 0.05f, 1.0f);
        renderer.DrawArc(pos, kHandleRadius, 0.0f, glm::two_pi<float>(),
                         fill, true, 1.0f, false, 0.0f);
        renderer.DrawArc(pos, kHandleRadius, 0.0f, glm::two_pi<float>(),
                         m_curveColor, false, active ? 2.0f : 1.5f, false,
                         active ? m_glowIntensity : m_glowIntensity * 0.35f);
    }

    RenderOverlay(renderer);
}

void CurveView::OnMouseDown(float x, float y) {
    if (!IsInsideBounds(x, y)) return;
    m_didDrag       = false;
    m_draggingIndex = HitTestPoint(x, y);
    if (m_draggingIndex >= 0) {
        m_dragSnapshot = m_points[m_draggingIndex];
    } else {
        glm::vec2 norm = ToNormalized(x, y);
        AddPoint(norm.x, norm.y);
        m_draggingIndex = HitTestPoint(x, y);
        NotifyChanged();
    }
}

void CurveView::OnMouseMove(float x, float y) {
    if (m_draggingIndex < 0) return;
    m_didDrag = true;
    CurvePoint& pt   = m_points[m_draggingIndex];
    glm::vec2   norm = ToNormalized(x, y);
    if (!pt.lockX) pt.x = norm.x;
    if (!pt.lockY) pt.y = norm.y;
    NotifyChanged();
}

void CurveView::OnMouseUp(float x, float y) {
    if (m_draggingIndex < 0) return;
    if (!m_didDrag) {
        const CurvePoint& pt = m_points[m_draggingIndex];
        if (!pt.lockX && m_points.size() > 2) {
            m_points.erase(m_points.begin() + m_draggingIndex);
            NotifyChanged();
        }
    } else {
        SortPoints();
        NotifyChanged();
    }
    m_draggingIndex = -1;
    m_didDrag       = false;
}

} // namespace oscilleon::gui