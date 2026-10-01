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
        m_width = width;
        m_height = height;

        m_assetManager = std::make_unique<AssetManager>();
        m_assetManager->Init();
        m_assetManager->LoadFont("jb-default.ttf", "Widgets", 20);
        m_assetManager->LoadFont("jb-default.ttf", "Labels", 45); /// header

        m_renderer = std::make_unique<Renderer>();
        m_renderer->SetAssetManager(m_assetManager.get());
        m_renderer->Init(width, height);

        CreateLayout();
        Resize(width, height);
    }

    void Editor::CreateLayout()
    {
        m_knobs.clear();
        m_lamps.clear();
        m_paramIds.clear();

        constexpr int kCols = 8;
        constexpr int kRows = 4;

        m_layout = std::make_unique<GridLayout>(kCols, kRows);

        for (int col = 0; col < kCols; ++col)
            m_layout->SetColWidth(col, GridUnit::Fraction(1));

        m_layout->SetRowHeight(0, GridUnit::Pixels(46));
        m_layout->SetRowHeight(1, GridUnit::Fraction(2.4f));
        m_layout->SetRowHeight(2, GridUnit::Fraction(1));
        m_layout->SetRowHeight(3, GridUnit::Fraction(1));

        ///m_layout->SetPadding(5, 5);
        m_layout->SetSpacing(1, 1);


        const glm::vec4 waveColor   { 0.85f, 0.90f, 0.95f, 1.0f }; /// whiteish
        const glm::vec4 gridColor   { 0.18f, 0.22f, 0.28f, 1.0f };
        const glm::vec4 grainColor  { 0.10f, 0.70f, 1.00f, 1.0f }; /// some blue
        const glm::vec4 utilColor   { 0.35f, 0.55f, 0.75f, 1.0f }; /// some muted blue
        const glm::vec4 writeColor  { 0.20f, 0.85f, 1.00f, 1.0f }; /// cyan

        struct ParamLayout {
            Steinberg::oscilleon::ParamID   id;
            int       col;
            int       row;
            glm::vec4 color;
        };

        const ParamLayout table[] = {
            { kParamDry,        0, 2, utilColor },
            { kParamBuffer,     1, 2, utilColor },
            { kParamGranulator, 2, 2, utilColor },
            { kParamMaxGrains,  3, 2, utilColor },
            { kParamInput,      4, 2, utilColor },
            { kParamFeedback,   5, 2, utilColor },
            { kParamBufferSize, 6, 2, utilColor },
            { kParamFreeze,     7, 2, utilColor },

            { kParamPosition,   0, 3, grainColor },
            { kParamIncrement,  1, 3, grainColor },
            { kParamRate,       2, 3, grainColor },
            { kParamLength,     3, 3, grainColor },
            { kParamPitch,      4, 3, grainColor },
            { kParamReverse,    5, 3, grainColor },
            { kParamPan,        6, 3, grainColor },
            { kParamVolume,     7, 3, grainColor },
        };
        m_toolbar = std::make_unique<Toolbar>();
        m_toolbar->SetCompanyName("EigenDSP");
        m_toolbar->SetPluginName("Granulator");
        m_layout->Add(m_toolbar.get(), 0, 0, kCols, 1);

        m_grainView = std::make_unique<GrainView>();
        m_grainView->SetWaveColor(waveColor);
        m_grainView->SetPrimaryColor(grainColor);
        m_grainView->SetAccentColor({ 0.20f, 0.85f, 1.00f, 1.0f });
        m_grainView->SetWriteHeadColor(writeColor);
        m_grainView->SetGridColor(gridColor);
        m_grainView->SetTextColor({ 0.70f, 0.80f, 0.95f, 1.0f });
        m_layout->Add(m_grainView.get(), 0, 1, kCols, 1);

        for (const auto& pl : table) {
            const auto& info = getParamInfo(pl.id);
            bool        discrete = info.stepCount > 0;
            size_t      steps = discrete ? static_cast<size_t>(info.max - info.min + 1) : 0;

            auto knob = std::make_unique<Knob>(
                discrete ? KnobMode::DISCRETE : KnobMode::CONTINUOUS,
                discrete ? KnobIndicatorStyle::DISCRETE_STEPS : KnobIndicatorStyle::BASIC,
                steps
            );

            knob->SetName(Utf16ToUtf8(info.name));
            knob->SetArcColor(pl.color);
            knob->SetRange(static_cast<float>(info.min), static_cast<float>(info.max));

            if (m_controller) {
                float norm = static_cast<float>(m_controller->getParamNormalized(pl.id));
                float actual = static_cast<float>(info.min + norm * (info.max - info.min));
                knob->SetValue(actual);
            }

            knob->SetOnValueChanged([this, id = pl.id,
                min = (float)info.min,
                max = (float)info.max](float value) {
                    double norm = static_cast<double>((value - min) / (max - min));
                    if (m_controller) {
                        m_controller->setParamNormalized(id, norm);
                        m_controller->performEdit(id, norm);
                    }
                });

            m_layout->Add(knob.get(), pl.col, pl.row);
            m_knobs.push_back(std::move(knob));
            m_paramIds.push_back(pl.id);
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

    void Editor::UpdateParameter(Vst::ParamID id, float normValue)
    {
        Steinberg::oscilleon::ParamID pid = static_cast<Steinberg::oscilleon::ParamID>(id);

        for (size_t i = 0; i < m_paramIds.size(); ++i) {
            if (m_paramIds[i] == pid) {
                m_knobs[i]->UpdateValue(normValue);
                break;
            }
        }

        if (!m_grainView) return;

        const auto& info = getParamInfo(pid);
        float       actual = static_cast<float>(info.min + normValue * (info.max - info.min));

        switch (pid) {
        case kParamBufferSize:
            if (m_grainView) {
                m_grainView->SetBufferSize(actual);
                m_grainView->NotifyBufferCleared();
            }
            break;

        case kParamFreeze:
            if (m_grainView) m_grainView->SetFreeze(actual >= 0.5f);
            break;
        case kParamLength:     m_grainView->SetGrainSize(actual);  break;
        case kParamRate:       m_grainView->SetDensity(actual);    break;
        case kParamPosition:   m_grainView->SetPosition(actual);   break;
        case kParamIncrement:  m_grainView->SetIncrement(actual);  break;
        case kParamPitch:      m_grainView->SetPitch(actual);      break;
        case kParamReverse:    m_grainView->SetReverse(actual);    break;
        case kParamPan:        m_grainView->SetPan(actual);        break;
        case kParamGranulator: m_grainView->SetMix(actual);        break;
        default: break;
        }
    }

    void Editor::Render() {
        if (!m_renderer) return;
        m_renderer->BeginFrame({ 0.04f, 0.04f, 0.05f, 1.0f });

        if (m_toolbar)   m_toolbar->Render(*m_renderer);
        if (m_grainView) m_grainView->Render(*m_renderer);
        for (auto& knob : m_knobs) knob->Render(*m_renderer);

        m_renderer->EndFrame();
    }

    void Editor::SetWaveformData(const float* L, const float* R,
                                Steinberg::uint32 count,
                                float bufferSizeSec,
                                Steinberg::uint32 writePos)
    {
        if (m_grainView)
            m_grainView->SetWaveformData(L, R, count, bufferSizeSec, writePos);
    }


    void Editor::UpdateWidgets(float dt) {
        if (m_grainView) m_grainView->Update(dt);
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

    void Editor::CancelDrag() {
        if (m_activeKnob) {
            m_activeKnob->OnMouseUp(-9999.f, -9999.f);
            m_activeKnob = nullptr;
        }
    }
}
