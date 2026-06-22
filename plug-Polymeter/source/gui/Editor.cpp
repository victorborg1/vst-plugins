#include "Editor.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include "../cids.h"
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
    m_width = width;
    m_height = height;

    m_assetManager = std::make_unique<AssetManager>();
    m_assetManager->Init();
    m_assetManager->LoadFont("Aldrich.ttf", "Toolbar", 80);
    m_assetManager->LoadFont("Aldrich.ttf", "Widgets", 20);
    m_assetManager->LoadFont("Aldrich.ttf", "Labels", 45);

    m_renderer = std::make_unique<Renderer>();
    m_renderer->SetAssetManager(m_assetManager.get());
    m_renderer->Init(width, height);

    CreateLayout();
    Resize(width, height);
}

void Editor::CreateLayout() {
    constexpr int kCols = 4;
    constexpr int kRows = 4;

    m_layout = std::make_unique<GridLayout>(kCols, kRows);

    for (int col = 0; col < kCols; ++col)
        m_layout->SetColWidth(col, GridUnit::Fraction(1));

    m_layout->SetRowHeight(0, GridUnit::Pixels(50));
    m_layout->SetRowHeight(1, GridUnit::Pixels(60));
    m_layout->SetRowHeight(2, GridUnit::Fraction(1));
    m_layout->SetRowHeight(3, GridUnit::Fraction(1));

    //m_layout->SetPadding(5, 5);
    m_layout->SetSpacing(1, 1);

    glm::vec4 rhythmColor{ 0.10f, 0.70f, 1.00f, 1.0f };
    glm::vec4 globalColor{ 0.10f, 0.70f, 1.00f, 1.0f };
    glm::vec4 triggerColor{ 0.10f, 0.70f, 1.00f, 1.0f };
    glm::vec4 syncColor{ 0.10f, 1.00f, 0.10f, 1.0f };

    m_toolbar = std::make_unique<Toolbar>();
    m_toolbar->SetCompanyName("EigenDSP");
    m_toolbar->SetPluginName("Polymeter");
    m_layout->Add(m_toolbar.get(), 0, 0, kCols, 1);

    // Trigger lamps
    for (int i = 0; i < 4; ++i) {
        auto lamp = std::make_unique<Lamp>();
        lamp->SetOnColor(triggerColor);
        lamp->SetSyncColor(syncColor);
        m_layout->Add(lamp.get(), i, 1);
        m_lamps.push_back(std::move(lamp));
    }

    // Rhythm knobs
    const Steinberg::oscilleon::ParamID rhythmParams[] = {
        kParamRhythmA, kParamRhythmB, kParamRhythmC, kParamRhythmD
    };
    for (int i = 0; i < 4; ++i) {
        Steinberg::oscilleon::ParamID id = rhythmParams[i];
        const auto& info = getParamInfo(id);
        bool discrete = info.stepCount > 0;
        size_t steps = discrete ? static_cast<size_t>(info.max - info.min + 1) : 0;

        auto knob = std::make_unique<Knob>(
            discrete ? KnobMode::DISCRETE : KnobMode::CONTINUOUS,
            discrete ? KnobIndicatorStyle::DISCRETE_STEPS : KnobIndicatorStyle::BASIC,
            steps
        );
        knob->SetName(Utf16ToUtf8(info.name));
        knob->SetArcColor(rhythmColor);
        knob->SetRange(static_cast<float>(info.min), static_cast<float>(info.max));

        if (m_controller) {
            float norm = static_cast<float>(m_controller->getParamNormalized(id));
            float actual = static_cast<float>(fromNormalized(norm, info.min, info.max));
            knob->SetValue(actual);
        }

        knob->SetOnValueChanged([this, id, min = (float)info.min, max = (float)info.max](float value) {
            float norm = (value - min) / (max - min);
            if (m_controller) {
                m_controller->setParamNormalized(id, norm);
                m_controller->performEdit(id, norm);
            }
        });

        m_layout->Add(knob.get(), i, 2);
        m_knobs.push_back(std::move(knob));
        m_paramIds.push_back(id);
    }

    // Global knobs
    const Steinberg::oscilleon::ParamID globalParams[] = {
        kParamNoteLength, kParamVelocity, kParamBarSize, kParamSwing
    };
    for (int i = 0; i < 4; ++i) {
        Steinberg::oscilleon::ParamID id = globalParams[i];
        const auto& info = getParamInfo(id);
        bool discrete = info.stepCount > 0;
        size_t steps = discrete ? static_cast<size_t>(info.max - info.min + 1) : 0;

        auto knob = std::make_unique<Knob>(
            discrete ? KnobMode::DISCRETE : KnobMode::CONTINUOUS,
            discrete ? KnobIndicatorStyle::DISCRETE_STEPS : KnobIndicatorStyle::BASIC,
            steps
        );
        knob->SetName(Utf16ToUtf8(info.name));
        knob->SetArcColor(globalColor);
        knob->SetRange(static_cast<float>(info.min), static_cast<float>(info.max));

        if (m_controller) {
            float norm = static_cast<float>(m_controller->getParamNormalized(id));
            float actual = static_cast<float>(fromNormalized(norm, info.min, info.max));
            knob->SetValue(actual);
        }

        knob->SetOnValueChanged([this, id, min = (float)info.min, max = (float)info.max](float value) {
            float norm = (value - min) / (max - min);
            if (m_controller) {
                m_controller->setParamNormalized(id, norm);
                m_controller->performEdit(id, norm);
            }
        });

        m_layout->Add(knob.get(), i, 3);
        m_knobs.push_back(std::move(knob));
        m_paramIds.push_back(id);
    }
}

void Editor::Resize(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_renderer) m_renderer->Resize(width, height);
    if (m_layout) {
        m_layout->SetBounds(0, 0, width, height);
        m_layout->DoLayout();
    }
}

void Editor::UpdateParameter(Vst::ParamID id, float valueNormalized) {
    
    for (size_t i = 0; i < m_paramIds.size(); ++i) {
        if (m_paramIds[i] == id) {
            m_knobs[i]->UpdateValue(valueNormalized);
            break;
        }
    }

    int lampIndex = -1;
    switch (id) {
        case kParamTriggerA: lampIndex = 0; break;
        case kParamTriggerB: lampIndex = 1; break;
        case kParamTriggerC: lampIndex = 2; break;
        case kParamTriggerD: lampIndex = 3; break;
        default: return;
    }

    if (lampIndex >= 0 && lampIndex < static_cast<int>(m_lamps.size())) {
        LampState state = (valueNormalized >= 1.0f) ? LampState::ON : LampState::OFF;
        m_lamps[lampIndex]->Flash(state);
    }
}

void Editor::Render() {
    if (!m_renderer) return;
    m_renderer->BeginFrame({ 0.05f, 0.05f, 0.05f, 1.0f });

    for (auto& lamp : m_lamps) lamp->Render(*m_renderer);
    for (auto& knob : m_knobs) knob->Render(*m_renderer);
    if (m_toolbar) m_toolbar->Render(*m_renderer);

    m_renderer->EndFrame();
}

void Editor::UpdateWidgets(float dt) {
    for (auto& lamp : m_lamps) lamp->Update(dt);
}

void Editor::OnMouseDown(float x, float y) {
    for (auto& knob : m_knobs) knob->OnMouseDown(x, y);
}

void Editor::OnMouseMove(float x, float y) {
    for (auto& knob : m_knobs) knob->OnMouseMove(x, y);
}

void Editor::OnMouseUp(float x, float y) {
    for (auto& knob : m_knobs) knob->OnMouseUp(x, y);
}

void Editor::CancelDrag() {
    for (auto& knob : m_knobs) knob->OnMouseUp(-9999.f, -9999.f);
}

void Editor::SetWaveformData(const float*, const float*, Steinberg::uint32, float, Steinberg::uint32) {}

} // namespace oscilleon::gui