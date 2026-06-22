// ArpGrid.cpp
#include "ArpGrid.h"

#include <algorithm>

namespace oscilleon::gui {

bool ArpGrid::GetCell(int step, int row) const {
    if (step < 0 || step >= StepCount) return false;
    if (row < 0 || row >= RowCount) return false;

    return m_grid[step][row];
}

void ArpGrid::SetCell(int step, int row, bool enabled) {
    if (step < 0 || step >= StepCount) return;
    if (row < 0 || row >= RowCount) return;

    m_grid[step][row] = enabled;
}

void ArpGrid::Clear() {
    for (auto& column : m_grid) {
        column.fill(false);
    }
}

void ArpGrid::SetPlayhead(int step) {
    m_playhead = std::clamp(step, 0, m_loopLength - 1);
}

int ArpGrid::GetPlayhead() const {
    return m_playhead;
}

void ArpGrid::SetLoopLength(int length) {
    m_loopLength = std::clamp(length, 1, StepCount);

    if (m_playhead >= m_loopLength) {
        m_playhead = 0;
    }
}

int ArpGrid::GetLoopLength() const {
    return m_loopLength;
}

void ArpGrid::SetActiveColor(const glm::vec4& color) {
    m_activeColor = color;
}

void ArpGrid::SetOnChanged(std::function<void(int, int, bool)> callback) {
    m_onChanged = std::move(callback);
}

const ArpGrid::Grid& ArpGrid::GetGrid() const {
    return m_grid;
}

float ArpGrid::CellWidth() const {
    return GetGridRect().w / float(StepCount);
}

float ArpGrid::CellHeight() const {
    return GetGridRect().h / float(RowCount);
}

ArpGrid::Rect ArpGrid::GetGridRect() const {
    return {
        m_x + OuterPadding,
        m_y + OuterPadding,
        m_width  - OuterPadding * 2.0f,
        m_height - OuterPadding * 2.0f - LoopBarHeight - LoopBarGap
    };
}

ArpGrid::Rect ArpGrid::GetCellRect(int step, int row) const {
    const auto grid = GetGridRect();

    return {
        grid.x + step * CellWidth() + CellGap * 0.5f,
        grid.y + row  * CellHeight() + CellGap * 0.5f,
        CellWidth() - CellGap,
        CellHeight() - CellGap
    };
}

ArpGrid::Rect ArpGrid::GetLoopHandleRect() const {
    const auto grid = GetGridRect();

    return {
        grid.x,
        grid.y + grid.h + LoopBarGap,
        grid.w,
        LoopBarHeight
    };
}

bool ArpGrid::GetCellAt(float x, float y, int& step, int& row) const {
    const auto grid = GetGridRect();

    if (x < grid.x || x >= grid.x + grid.w) return false;
    if (y < grid.y || y >= grid.y + grid.h) return false;

    step = int((x - grid.x) / CellWidth());
    row  = int((y - grid.y) / CellHeight());

    step = std::clamp(step, 0, StepCount - 1);
    row  = std::clamp(row, 0, RowCount - 1);

    return true;
}

bool ArpGrid::IsOverLoopHandle(float x, float y) const {
    const auto r = GetLoopHandleRect();

    return
        x >= r.x &&
        x <= r.x + r.w &&
        y >= r.y &&
        y <= r.y + r.h;
}

void ArpGrid::PaintCell(int step, int row, bool state) {
    if (step == m_lastStep && row == m_lastRow) {
        return;
    }

    m_lastStep = step;
    m_lastRow  = row;

    m_grid[step][row] = state;

    if (m_onChanged) {
        m_onChanged(step, row, state);
    }
}

void ArpGrid::OnMouseDown(float x, float y) {
    if (IsOverLoopHandle(x, y)) {
        m_dragMode = DragMode::ResizeLoop;
        OnMouseMove(x, y);
        return;
    }

    int step;
    int row;

    if (!GetCellAt(x, y, step, row)) {
        return;
    }

    m_lastStep = -1;
    m_lastRow  = -1;

    m_paintValue = true; // left click paint.
    m_dragMode = DragMode::Paint;

    PaintCell(step, row, m_paintValue);
}

void ArpGrid::OnMouseMove(float x, float y) {
    if (m_dragMode == DragMode::ResizeLoop) {
        const auto grid = GetGridRect();

        int steps = int((x - grid.x) / CellWidth()) + 1;

        SetLoopLength(steps);

        return;
    }

    if (m_dragMode != DragMode::Paint) {
        return;
    }

    int step;
    int row;

    if (!GetCellAt(x, y, step, row)) {
        return;
    }

    PaintCell(step, row, m_paintValue);
}

void ArpGrid::OnMouseUp(float, float) {
    m_dragMode = DragMode::None;
}

void ArpGrid::OnRightMouseDown(float x, float y) {
    if (IsOverLoopHandle(x, y)) {
        return;
    }

    int step;
    int row;

    if (!GetCellAt(x, y, step, row)) {
        return;
    }

    m_lastStep = -1;
    m_lastRow  = -1;

    m_paintValue = false;
    m_dragMode = DragMode::Erase;

    PaintCell(step, row, m_paintValue);
}

void ArpGrid::OnRightMouseMove(float x, float y) {
    if (m_dragMode != DragMode::Erase) {
        return;
    }

    int step;
    int row;

    if (!GetCellAt(x, y, step, row)) {
        return;
    }

    PaintCell(step, row, false);
}

void ArpGrid::OnRightMouseUp(float, float) {
    m_dragMode = DragMode::None;
}

void ArpGrid::UpdatePlayhead() {
    auto now = std::chrono::steady_clock::now();

    float dt =
        std::chrono::duration<float>(now - m_lastFrame).count();

    m_lastFrame = now;

    dt = std::min(dt, 0.1f);

    m_stepTimer += dt;

    while (m_stepTimer >= StepTime) {
        m_stepTimer -= StepTime;
        m_playhead = (m_playhead + 1) % m_loopLength;
    }
}

void ArpGrid::Render(Renderer& renderer) {
    UpdatePlayhead();

    renderer.DrawRect(
        { m_x, m_y },
        { m_width, m_height },
        { 0.08f, 0.08f, 0.08f, 1.0f });

    for (int step = 0; step < StepCount; ++step) {
        const bool inLoop   = step < m_loopLength;
        const bool playhead = step == m_playhead;
        const bool beat     = step % 4 == 0;

        for (int row = 0; row < RowCount; ++row) {
            const bool enabled = m_grid[step][row];

            const auto rect = GetCellRect(step, row);

            glm::vec4 color;

            if (!inLoop) {
                if (enabled) {
                    color = {
                        m_activeColor.r * 0.35f,
                        m_activeColor.g * 0.35f,
                        m_activeColor.b * 0.35f,
                        1.0f
                    };
                }
                else {
                    color = { 0.05f, 0.05f, 0.05f, 1.0f };
                }
            }
            else if (enabled) {
                color = playhead
                    ? glm::vec4(
                        std::min(m_activeColor.r * 1.3f, 1.0f),
                        std::min(m_activeColor.g * 1.3f, 1.0f),
                        std::min(m_activeColor.b * 1.3f, 1.0f),
                        1.0f)
                    : m_activeColor;
            }
            else if (playhead) {
                color = { 0.22f, 0.22f, 0.22f, 1.0f };
            }
            else {
                color = beat
                    ? glm::vec4(0.15f, 0.15f, 0.15f, 1.0f)
                    : glm::vec4(0.11f, 0.11f, 0.11f, 1.0f);
            }

            renderer.DrawRect(
                { rect.x, rect.y },
                { rect.w, rect.h },
                color);
        }
    }

    const auto loopBar = GetLoopHandleRect();

    renderer.DrawRect(
        { loopBar.x, loopBar.y },
        { loopBar.w, loopBar.h },
        { 0.08f, 0.08f, 0.08f, 1.0f });

    renderer.DrawRect(
        { loopBar.x, loopBar.y },
        { CellWidth() * m_loopLength, loopBar.h },
        m_activeColor);
}

} // namespace oscilleon::gui