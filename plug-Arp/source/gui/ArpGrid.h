#pragma once

#include <array>
#include <chrono>
#include <functional>

#include <glm/glm.hpp>

#include "Widgets/Widget.h"

namespace oscilleon::gui {

class ArpGrid : public Widget {
public:
    static constexpr int StepCount = 16;
    static constexpr int RowCount  = 8;

    using Grid = std::array<std::array<bool, RowCount>, StepCount>;

    void Render(Renderer& renderer) override;

    void OnMouseDown(float x, float y) override;
    void OnMouseMove(float x, float y) override;
    void OnMouseUp(float x, float y) override;

    void OnRightMouseDown(float x, float y);
    void OnRightMouseMove(float x, float y);
    void OnRightMouseUp(float x, float y);

    bool GetCell(int step, int row) const;
    void SetCell(int step, int row, bool enabled);

    void Clear();

    void SetPlayhead(int step);
    int GetPlayhead() const;

    void SetLoopLength(int length);
    int GetLoopLength() const;

    void SetActiveColor(const glm::vec4& color);

    void SetOnChanged(std::function<void(int, int, bool)> callback);

    const Grid& GetGrid() const;

private:
    struct Rect {
        float x;
        float y;
        float w;
        float h;
    };

    enum class DragMode {
        None,
        Paint,
        Erase,
        ResizeLoop
    };

private:
    void UpdatePlayhead();

    bool GetCellAt(float x, float y, int& step, int& row) const;
    bool IsOverLoopHandle(float x, float y) const;

    void PaintCell(int step, int row, bool state);

    Rect GetGridRect() const;
    Rect GetCellRect(int step, int row) const;
    Rect GetLoopHandleRect() const;

    float CellWidth() const;
    float CellHeight() const;

private:
    Grid m_grid{};

    std::function<void(int, int, bool)> m_onChanged;

    glm::vec4 m_activeColor{ 0.28f, 0.75f, 1.0f, 1.0f };

    int m_playhead   = -1;
    int m_loopLength = StepCount;

    DragMode m_dragMode = DragMode::None;

    bool m_paintValue = false;

    int m_lastStep = -1;
    int m_lastRow  = -1;

    std::chrono::steady_clock::time_point m_lastFrame =
        std::chrono::steady_clock::now();

    float m_stepTimer = 0.0f;

private:
    static constexpr float OuterPadding  = 8.0f;
    static constexpr float CellGap       = 2.0f;
    static constexpr float LoopBarHeight = 12.0f;
    static constexpr float LoopBarGap    = 6.0f;

    static constexpr float BPM      = 120.0f;
    static constexpr float StepTime = 60.0f / BPM / 4.0f;
};

} // namespace oscilleon::gui