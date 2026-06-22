#include "GridLayout.h"
#include <algorithm>
#include <numeric>

namespace oscilleon::gui {

GridLayout::GridLayout(int cols, int rows)
    : m_cols(cols), m_rows(rows) {
    m_colUnits.resize(cols, GridUnit::Fraction(1));
    m_rowUnits.resize(rows, GridUnit::Fraction(1));
    m_colWidths.resize(cols);
    m_rowHeights.resize(rows);
    m_colPositions.resize(cols + 1);
    m_rowPositions.resize(rows + 1);
}

void GridLayout::SetCols(int cols) {
    m_cols = cols;
    m_colUnits.resize(cols, GridUnit::Fraction(1));
    m_colWidths.resize(cols);
    m_colPositions.resize(cols + 1);
}

void GridLayout::SetRows(int rows) {
    m_rows = rows;
    m_rowUnits.resize(rows, GridUnit::Fraction(1));
    m_rowHeights.resize(rows);
    m_rowPositions.resize(rows + 1);
}

void GridLayout::SetColWidth(int col, GridUnit unit) {
    if (col >= 0 && col < m_cols) {
        m_colUnits[col] = unit;
    }
}

void GridLayout::SetRowHeight(int row, GridUnit unit) {
    if (row >= 0 && row < m_rows) {
        m_rowUnits[row] = unit;
    }
}

void GridLayout::SetPadding(float padding) {
    SetPadding(padding, padding, padding, padding);
}

void GridLayout::SetPadding(float horizontal, float vertical) {
    SetPadding(vertical, horizontal, vertical, horizontal);
}

void GridLayout::SetPadding(float top, float right, float bottom, float left) {
    m_paddingTop = top;
    m_paddingRight = right;
    m_paddingBottom = bottom;
    m_paddingLeft = left;
}

void GridLayout::SetSpacing(float spacing) {
    SetSpacing(spacing, spacing);
}

void GridLayout::SetSpacing(float horizontal, float vertical) {
    m_spacingX = horizontal;
    m_spacingY = vertical;
}

void GridLayout::SetBounds(float x, float y, float width, float height) {
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void GridLayout::Add(Widget* widget, int col, int row, int colSpan, int rowSpan) {
    if (!widget || col < 0 || col >= m_cols || row < 0 || row >= m_rows) return;

    Cell cell;
    cell.widget = widget;
    cell.col = col;
    cell.row = row;
    cell.colSpan = std::min(colSpan, m_cols - col);
    cell.rowSpan = std::min(rowSpan, m_rows - row);

    m_cells.push_back(cell);
}

void GridLayout::Remove(Widget* widget) {
    m_cells.erase(
        std::remove_if(m_cells.begin(), m_cells.end(),
            [widget](const Cell& cell) { return cell.widget == widget; }),
        m_cells.end()
    );
}

void GridLayout::Clear() {
    m_cells.clear();
}

void GridLayout::ComputeColWidths() {
    float totalFixed = 0;
    float totalFraction = 0;
    float totalPercent = 0;

    for (const auto& unit : m_colUnits) {
        if (unit.type == GridUnit::Type::Pixels) totalFixed += unit.value;
        else if (unit.type == GridUnit::Type::Fraction) totalFraction += unit.value;
        else if (unit.type == GridUnit::Type::Percent) totalPercent += unit.value;
    }

    float availableWidth = m_width - m_paddingLeft - m_paddingRight - (m_cols - 1) * m_spacingX;
    float percentWidth = availableWidth * (totalPercent / 100.0f);
    float remainingWidth = availableWidth - totalFixed - percentWidth;

    m_colPositions[0] = m_x + m_paddingLeft;

    for (int i = 0; i < m_cols; i++) {
        const auto& unit = m_colUnits[i];

        if (unit.type == GridUnit::Type::Pixels) {
            m_colWidths[i] = unit.value;
        }
        else if (unit.type == GridUnit::Type::Percent) {
            m_colWidths[i] = availableWidth * (unit.value / 100.0f);
        }
        else if (unit.type == GridUnit::Type::Fraction) {
            m_colWidths[i] = remainingWidth * (unit.value / totalFraction);
        }

        if (m_colWidths[i] < 0) m_colWidths[i] = 0;

        m_colPositions[i + 1] = m_colPositions[i] + m_colWidths[i] + m_spacingX;
    }
}

void GridLayout::ComputeRowHeights() {
    float totalFixed = 0;
    float totalFraction = 0;
    float totalPercent = 0;

    for (const auto& unit : m_rowUnits) {
        if (unit.type == GridUnit::Type::Pixels) totalFixed += unit.value;
        else if (unit.type == GridUnit::Type::Fraction) totalFraction += unit.value;
        else if (unit.type == GridUnit::Type::Percent) totalPercent += unit.value;
    }

    float availableHeight = m_height - m_paddingTop - m_paddingBottom - (m_rows - 1) * m_spacingY;
    float percentHeight = availableHeight * (totalPercent / 100.0f);
    float remainingHeight = availableHeight - totalFixed - percentHeight;

    m_rowPositions[0] = m_y + m_paddingTop;

    for (int i = 0; i < m_rows; i++) {
        const auto& unit = m_rowUnits[i];

        if (unit.type == GridUnit::Type::Pixels) {
            m_rowHeights[i] = unit.value;
        }
        else if (unit.type == GridUnit::Type::Percent) {
            m_rowHeights[i] = availableHeight * (unit.value / 100.0f);
        }
        else if (unit.type == GridUnit::Type::Fraction) {
            m_rowHeights[i] = remainingHeight * (unit.value / totalFraction);
        }

        if (m_rowHeights[i] < 0) m_rowHeights[i] = 0;

        m_rowPositions[i + 1] = m_rowPositions[i] + m_rowHeights[i] + m_spacingY;
    }
}

void GridLayout::UpdateCellPositions() {
    for (auto& cell : m_cells) {
        float x = m_colPositions[cell.col];
        float y = m_rowPositions[cell.row];

        float width = 0;
        for (int i = 0; i < cell.colSpan; i++) {
            width += m_colWidths[cell.col + i];
            if (i < cell.colSpan - 1) width += m_spacingX;
        }

        float height = 0;
        for (int i = 0; i < cell.rowSpan; i++) {
            height += m_rowHeights[cell.row + i];
            if (i < cell.rowSpan - 1) height += m_spacingY;
        }

        cell.widget->SetPosition(x, y);
        cell.widget->SetSize(width, height);
    }
}

void GridLayout::DoLayout() {
    ComputeColWidths();
    ComputeRowHeights();
    UpdateCellPositions();
}

float GridLayout::GetColWidth(int col) const {
    return (col >= 0 && col < m_colWidths.size()) ? m_colWidths[col] : 0;
}

float GridLayout::GetRowHeight(int row) const {
    return (row >= 0 && row < m_rowHeights.size()) ? m_rowHeights[row] : 0;
}

}