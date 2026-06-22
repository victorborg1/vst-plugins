#pragma once
#include <vector>
#include <memory>
#include <functional>
#include "../Widgets/Widget.h"
#include "GridUnit.h"

namespace oscilleon::gui {

class GridLayout {
public:
    GridLayout(int cols = 1, int rows = 1);

    void SetCols(int cols);
    void SetRows(int rows);
    void SetColWidth(int col, GridUnit unit);
    void SetRowHeight(int row, GridUnit unit);

    void SetPadding(float padding);
    void SetPadding(float horizontal, float vertical);
    void SetPadding(float top, float right, float bottom, float left);
    void SetSpacing(float spacing);
    void SetSpacing(float horizontal, float vertical);

    void SetBounds(float x, float y, float width, float height);

    void Add(Widget* widget, int col, int row, int colSpan = 1, int rowSpan = 1);
    void Remove(Widget* widget);
    void Clear();

    void DoLayout();

    int GetCols() const { return m_cols; }
    int GetRows() const { return m_rows; }
    float GetColWidth(int col) const;
    float GetRowHeight(int row) const;
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetWidth() const { return m_width; }
    float GetHeight() const { return m_height; }

private:
    void ComputeColWidths();
    void ComputeRowHeights();
    void UpdateCellPositions();

private:
    struct Cell {
        Widget* widget = nullptr;
        int col = 0;
        int row = 0;
        int colSpan = 1;
        int rowSpan = 1;
    };

    int m_cols;
    int m_rows;

    float m_x = 0;
    float m_y = 0;
    float m_width = 0;
    float m_height = 0;

    float m_paddingTop = 0;
    float m_paddingRight = 0;
    float m_paddingBottom = 0;
    float m_paddingLeft = 0;

    float m_spacingX = 0;
    float m_spacingY = 0;

    std::vector<GridUnit> m_colUnits;
    std::vector<GridUnit> m_rowUnits;
    std::vector<Cell> m_cells;
    std::vector<float> m_colWidths;
    std::vector<float> m_rowHeights;
    std::vector<float> m_colPositions;
    std::vector<float> m_rowPositions;
};

}
