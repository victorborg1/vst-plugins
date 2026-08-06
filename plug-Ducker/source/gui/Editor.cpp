#include "Editor.h"
#include <glm/gtc/matrix_transform.hpp>
#include "../Parameter.h"
#include "../Controller.h"
#include "../SharedState.h"
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
    m_assetManager->LoadFont("jb-default.ttf", "Widgets", 20);
    m_assetManager->LoadFont("jb-default.ttf", "Labels", 45); // header
    m_renderer = std::make_unique<Renderer>();
    m_renderer->SetAssetManager(m_assetManager.get());
    m_renderer->Init(width, height);

    CreateLayout();
    Resize(width, height);
}

void Editor::CreateLayout() {
    m_knobs.clear();
    m_paramIds.clear();

    constexpr int kCols = 4;
    constexpr int kRows = 3;

    m_layout = std::make_unique<GridLayout>(kCols, kRows);

    for (int col = 0; col < kCols; ++col)
        m_layout->SetColWidth(col, GridUnit::Fraction(1));

    m_layout->SetRowHeight(0, GridUnit::Pixels(44));
    m_layout->SetRowHeight(1, GridUnit::Fraction(1));
    m_layout->SetRowHeight(2, GridUnit::Pixels(160));

    //m_layout->SetPadding(5, 5);
    m_layout->SetSpacing(1, 1);

    m_toolbar = std::make_unique<Toolbar>();
    m_toolbar->SetCompanyName("EigenDSP");
    m_toolbar->SetPluginName("Ducker");
    m_layout->Add(m_toolbar.get(), 0, 0, kCols, 1);

    m_lfo = std::make_unique<Lfo>();
    m_lfo->SetLabel("LFO SHAPE");
    m_lfo->SetBeats(4);
    m_lfo->SetSubBeats(4);
    m_lfo->SetBtnFont("Widgets");
    m_lfo->SetCurveColor({ 1.0f, 0.18f, 0.18f, 1.0f });
    m_lfo->SetGlowIntensity(12.0f);
    m_lfo->SetLineThickness(2.5f);
    m_lfo->AddPoint(0.0f, 0.0f, true);
    m_lfo->AddPoint(1.0f, 0.0f, true);
    m_lfo->SetOnChanged([this](const std::vector<CurvePoint>&) {
        BakeCurveToSharedState();
        });
    m_layout->Add(m_lfo.get(), 0, 1, kCols, 1);

    BakeCurveToSharedState();

    struct KnobDef {
        Steinberg::oscilleon::ParamID id;
        glm::vec4                     color;
    };

    const KnobDef knobDefs[] = {
        { kParamAmount,  { 0.4f, 0.6f, 1.0f, 1.0f } },
        { kParamAttack,  { 0.4f, 0.6f, 1.0f, 1.0f } },
        { kParamRelease, { 0.4f, 0.6f, 1.0f, 1.0f } },
        { kParamMix,     { 0.4f, 0.6f, 1.0f, 1.0f } },
    };

    for (int col = 0; col < kCols; ++col) {
        const auto& def = knobDefs[col];
        const auto& info = getParamInfo(def.id);

        auto knob = std::make_unique<Knob>(KnobMode::CONTINUOUS, KnobIndicatorStyle::BASIC, 0);
        knob->SetName(Utf16ToUtf8(info.name));
        knob->SetArcColor(def.color);
        knob->SetRange(static_cast<float>(info.min), static_cast<float>(info.max));

        if (m_controller) {
            float norm = static_cast<float>(m_controller->getParamNormalized(def.id));
            float actual = static_cast<float>(info.min + norm * (info.max - info.min));
            knob->SetValue(actual);
        }

        knob->SetOnValueChanged([this, id = def.id,
            mn = (float)info.min,
            mx = (float)info.max](float value) {
                double norm = static_cast<double>((value - mn) / (mx - mn));
                if (m_controller) {
                    m_controller->setParamNormalized(id, norm);
                    m_controller->performEdit(id, norm);
                }
            });

        m_layout->Add(knob.get(), col, 2);
        m_knobs.push_back(std::move(knob));
        m_paramIds.push_back(static_cast<Steinberg::oscilleon::ParamID>(def.id));
    }
}

void Editor::BakeCurveToSharedState() {
    std::array<float, ::oscilleon::kCurveSampleCount> samples;
    for (int i = 0; i < ::oscilleon::kCurveSampleCount; ++i) {
        float nx = static_cast<float>(i) / static_cast<float>(::oscilleon::kCurveSampleCount - 1);
        samples[i] = m_lfo->Evaluate(nx);
    }
    ::oscilleon::SharedLfoState::instance().setCurve(samples);
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

void Editor::UpdateWidgets(float dt) {
    if (!m_lfo) return;

    if (!::oscilleon::SharedLfoState::instance().isPlaying()) {
        m_lfo->SetPlayheadPhase(-1.0f);
        return;
    }

    float phase = ::oscilleon::SharedLfoState::instance().getPhase();
    m_lfo->SetPlayheadPhase(phase);
}

void Editor::Render() {
    if (!m_renderer) return;
    m_renderer->BeginFrame({ 0.01f, 0.01f, 0.01f, 1.0f });
    if (m_toolbar) m_toolbar->Render(*m_renderer);
    if (m_lfo)     m_lfo->Render(*m_renderer);
    for (auto& knob : m_knobs) knob->Render(*m_renderer);
    m_renderer->EndFrame();
}

void Editor::UpdateParameter(Vst::ParamID id, float normValue) {
    auto pid = static_cast<Steinberg::oscilleon::ParamID>(id);
    for (size_t i = 0; i < m_paramIds.size(); ++i) {
        if (m_paramIds[i] == pid) {
            m_knobs[i]->UpdateValue(normValue);
            break;
        }
    }
}

void Editor::OnMouseDown(float x, float y) {
    if (m_lfo) m_lfo->OnMouseDown(x, y);
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
    if (m_lfo) m_lfo->OnMouseMove(x, y);
    if (m_activeKnob) m_activeKnob->OnMouseMove(x, y);
}

void Editor::OnMouseUp(float x, float y) {
    if (m_lfo) m_lfo->OnMouseUp(x, y);
    if (m_activeKnob) {
        m_activeKnob->OnMouseUp(x, y);
        m_activeKnob = nullptr;
    }
}



void Editor::CancelDrag() {
    if (m_activeKnob) {
        m_activeKnob->OnMouseUp(-9999.f, -9999.f);
        m_activeKnob = nullptr;
    }
}

} // namespace oscilleon::gui
