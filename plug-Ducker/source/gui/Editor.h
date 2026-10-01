#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "AssetManager/AssetManager.h"
#include "Renderer/Renderer.h"
#include "Layout/GridLayout.h"
#include "../Parameter.h"
#include "Widgets/Knob.h"
#include "Widgets/Toolbar.h"
#include "Lfo.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace oscilleon::gui {

class Editor {
public:
    explicit Editor(Steinberg::Vst::EditController* controller);
    ~Editor();

    void Init(int width, int height);
    void Resize(int width, int height);
    void Render();
    void OnMouseDown(float x, float y);
    void OnMouseMove(float x, float y);
    void OnMouseUp(float x, float y);
    void OnRightMouseDown(float x, float y) {}
    void OnRightMouseMove(float x, float y) {}
    void OnRightMouseUp  (float x, float y) {}

    void CancelDrag();

    void UpdateWidgets(float dt);
    void UpdateParameter(Steinberg::Vst::ParamID id, float normValue);
    void SetWaveformData(const float* L, const float* R,
                        Steinberg::uint32 count,
                        float bufferSizeSec,
                        Steinberg::uint32 writePos) {}
private:
    void CreateLayout();
    void BakeCurveToSharedState();

    Steinberg::Vst::EditController* m_controller{ nullptr };
    std::unique_ptr<AssetManager>   m_assetManager;
    std::unique_ptr<Renderer>       m_renderer;
    std::unique_ptr<GridLayout>     m_layout;
    std::unique_ptr<Toolbar>        m_toolbar;
    std::unique_ptr<Lfo>            m_lfo;

    std::vector<std::unique_ptr<Knob>>         m_knobs;
    std::vector<Steinberg::oscilleon::ParamID> m_paramIds;
    
    Knob* m_activeKnob{ nullptr };
    int m_width{ 0 };
    int m_height{ 0 };
};

} // namespace oscilleon::gui
