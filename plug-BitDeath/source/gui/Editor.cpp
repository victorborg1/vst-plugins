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
    m_knobs.clear();
    m_paramIds.clear();

    constexpr int kCols = 2;
    constexpr int kRows = 4;

    m_layout = std::make_unique<GridLayout>(kCols, kRows);

    m_layout->SetColWidth(0, GridUnit::Fraction(1));
    m_layout->SetColWidth(1, GridUnit::Fraction(3));

    m_layout->SetRowHeight(0, GridUnit::Pixels(46));
    m_layout->SetRowHeight(1, GridUnit::Fraction(1));
    m_layout->SetRowHeight(2, GridUnit::Fraction(1));
    m_layout->SetRowHeight(3, GridUnit::Fraction(1));

    m_layout->SetSpacing(1, 1);

    m_toolbar = std::make_unique<Toolbar>();
    m_toolbar->SetCompanyName("EigenDSP");
    m_toolbar->SetPluginName("BitDeath");
    m_layout->Add(m_toolbar.get(), 0, 0, kCols, 1);

    const Steinberg::oscilleon::ParamID paramIDs[] = {
        kParamDownsample, kParamBitDepth, kParamMix
    };
    const glm::vec4 colours[] = {
        { 0.35f, 0.55f, 0.75f, 1.0f },
        { 0.10f, 0.70f, 1.00f, 1.0f },
        { 0.85f, 0.90f, 0.95f, 1.0f }
    };

    for (int i = 0; i < 3; ++i) {
        const auto& info = getParamInfo(paramIDs[i]);
        bool   discrete = (info.stepCount > 0);
        size_t steps    = discrete ? static_cast<size_t>(info.max - info.min + 1) : 0;

        auto knob = std::make_unique<Knob>(
            discrete ? KnobMode::DISCRETE    : KnobMode::CONTINUOUS,
            discrete ? KnobIndicatorStyle::DISCRETE_STEPS : KnobIndicatorStyle::BASIC,
            steps
        );

        knob->SetName(Utf16ToUtf8(info.name));
        knob->SetArcColor(colours[i]);
        knob->SetRange(static_cast<float>(info.min), static_cast<float>(info.max));

        if (m_controller) {
            float norm   = static_cast<float>(m_controller->getParamNormalized(paramIDs[i]));
            float actual = static_cast<float>(info.min + norm * (info.max - info.min));
            knob->SetValue(actual);
        }

        knob->SetOnValueChanged([this, id = paramIDs[i],
                                  min = static_cast<float>(info.min),
                                  max = static_cast<float>(info.max)](float value) {
            double norm = static_cast<double>((value - min) / (max - min));
            if (m_controller) {
                m_controller->setParamNormalized(id, norm);
                m_controller->performEdit(id, norm);
            }
        });

        m_layout->Add(knob.get(), 0, i + 1);
        m_knobs.push_back(std::move(knob));
        m_paramIds.push_back(paramIDs[i]);
    }

    m_waveform = std::make_unique<Waveform>();
    m_layout->Add(m_waveform.get(), 1, 1, 1, 3);
}

void Editor::SyncWaveformParams() {
    if (!m_waveform || !m_controller) return;

    auto denorm = [&](Steinberg::oscilleon::ParamID id) -> float {
        const auto& info = getParamInfo(id);
        float norm = static_cast<float>(m_controller->getParamNormalized(id));
        return static_cast<float>(info.min + norm * (info.max - info.min));
    };

    m_waveform->SetParams(
        denorm(kParamDownsample),
        denorm(kParamBitDepth),
        denorm(kParamMix)
    );
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

void Editor::UpdateParameter(Steinberg::Vst::ParamID id, float normValue) {
    auto pid = static_cast<Steinberg::oscilleon::ParamID>(id);
    for (size_t i = 0; i < m_paramIds.size(); ++i) {
        if (m_paramIds[i] == pid) {
            m_knobs[i]->UpdateValue(normValue);
            break;
        }
    }
}

void Editor::Render() {
    if (!m_renderer) return;
    SyncWaveformParams();
    m_renderer->BeginFrame({ 0.04f, 0.04f, 0.05f, 1.0f });

    if (m_toolbar)  m_toolbar->Render(*m_renderer);
    for (auto& knob : m_knobs) knob->Render(*m_renderer);
    if (m_waveform) m_waveform->Render(*m_renderer);
    
    m_renderer->EndFrame();
}

void Editor::OnMouseDown(float x, float y) {
    if (m_activeKnob) {
        m_activeKnob->OnMouseUp(x, y);
        m_activeKnob = nullptr;
    }
    for (auto& knob : m_knobs) {
        knob->OnMouseDown(x, y);
        if (knob->IsDragging()) {
            m_activeKnob = knob.get();
            break;
        }
    }
}

void Editor::OnMouseMove(float x, float y) {
    if (m_activeKnob)
        m_activeKnob->OnMouseMove(x, y);
}

void Editor::OnMouseUp(float x, float y) {
    if (m_activeKnob) {
        m_activeKnob->OnMouseUp(x, y);
        m_activeKnob = nullptr;
    }
}

void Editor::OnRightMouseDown(float x, float y) {

}

void Editor::OnRightMouseMove(float x, float y) {

}

void Editor::OnRightMouseUp(float x, float y) {

}

void Editor::CancelDrag() {
    if (m_activeKnob) {
        m_activeKnob->OnMouseUp(-9999.f, -9999.f);
        m_activeKnob = nullptr;
    }
}



} // namespace oscilleon::gui