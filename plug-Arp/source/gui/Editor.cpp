#include "Editor.h"
#include <glm/gtc/matrix_transform.hpp>
#include "../Parameter.h"
#include "../Controller.h"
#include "GuiTypes.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace Steinberg::oscilleon;

namespace oscilleon::gui {

Editor::Editor(EditController* controller) : m_controller(controller) {
    if (m_controller) m_controller->addRef();
}

Editor::~Editor() {
    if (m_controller) m_controller->release();
}

void Editor::Init(int width, int height) {
    m_width  = width;
    m_height = height;

    m_assetManager = std::make_unique<AssetManager>();
    m_assetManager->Init();
    m_assetManager->LoadFont("Aldrich.ttf", "Widgets", 20);
    m_assetManager->LoadFont("Aldrich.ttf", "Labels",  45);

    m_renderer = std::make_unique<Renderer>();
    m_renderer->SetAssetManager(m_assetManager.get());
    m_renderer->Init(width, height);

    CreateLayout();
    Resize(width, height);
}

void Editor::CreateLayout() {
    constexpr int kCols = 8;
    constexpr int kRows = 3;

    m_layout = std::make_unique<GridLayout>(kCols, kRows);

    for (int col = 0; col < kCols; ++col)
        m_layout->SetColWidth(col, GridUnit::Fraction(1));

    m_layout->SetRowHeight(0, GridUnit::Pixels(46));
    m_layout->SetRowHeight(1, GridUnit::Fraction(1));
    m_layout->SetRowHeight(2, GridUnit::Fraction(1));

    //m_layout->SetPadding(5, 5);
    m_layout->SetSpacing(1, 1);

    m_toolbar = std::make_unique<Toolbar>();
    m_toolbar->SetCompanyName("EigenDSP");
    m_toolbar->SetPluginName("Arp");
    m_layout->Add(m_toolbar.get(), 0, 0, kCols, 1);

    m_arpGrid = std::make_unique<ArpGrid>();
    m_arpGrid->SetActiveColor({ 0.28f, 0.75f, 1.0f, 1.0f });
    m_arpGrid->SetOnChanged([this](int step, int row, bool active) {
        // grid state lives in the editor; push to processor via message
        // when you're ready to implement that — see SetArpPlayhead() for
        // the reverse direction (processor -> editor)
    });
    m_layout->Add(m_arpGrid.get(), 0, 1, kCols, 1);
}

void Editor::Resize(int width, int height) {
    m_width  = width;
    m_height = height;
    if (m_renderer) m_renderer->Resize(width, height);
    if (m_layout) {
        m_layout->SetBounds(0, 0, width, height);
        m_layout->DoLayout();
    }
}

void Editor::Render() {
    if (!m_renderer) return;
    m_renderer->BeginFrame({ 0.04f, 0.04f, 0.05f, 1.0f });
    if (m_toolbar)  m_toolbar->Render(*m_renderer);
    if (m_arpGrid)  m_arpGrid->Render(*m_renderer);
    m_renderer->EndFrame();
}

void Editor::OnMouseDown(float x, float y) {
    if (m_arpGrid) m_arpGrid->OnMouseDown(x, y);
}

void Editor::OnMouseMove(float x, float y) {
    if (m_arpGrid) m_arpGrid->OnMouseMove(x, y);
}

void Editor::OnMouseUp(float x, float y) {
    if (m_arpGrid) m_arpGrid->OnMouseUp(x, y);
}

void Editor::OnRightMouseDown(float x, float y) {
    if (m_arpGrid) m_arpGrid->OnRightMouseDown(x, y);
}

void Editor::OnRightMouseMove(float x, float y) {
    if (m_arpGrid) m_arpGrid->OnRightMouseMove(x, y);
}

void Editor::OnRightMouseUp(float x, float y) {
    if (m_arpGrid) m_arpGrid->OnRightMouseUp(x, y);
}

void Editor::CancelDrag() {
    if (m_arpGrid) m_arpGrid->OnMouseUp(-1.f, -1.f);
}

void Editor::SetArpPlayhead(int step) {
    if (m_arpGrid) m_arpGrid->SetPlayhead(step);
}

} // namespace oscilleon::gui