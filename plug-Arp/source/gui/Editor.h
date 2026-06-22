#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "AssetManager/AssetManager.h"
#include "Layout/GridLayout.h"
#include "Renderer/Renderer.h"
#include "Widgets/Toolbar.h"
#include "ArpGrid.h"
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
    void OnMouseUp  (float x, float y);
    void OnRightMouseDown(float x, float y);
    void OnRightMouseMove(float x, float y);
    void OnRightMouseUp  (float x, float y);
    
    void CancelDrag ();
    void UpdateWidgets(float dt) {}

    void UpdateParameter(Steinberg::Vst::ParamID id, float normValue) {}
    void SetWaveformData(const float*, const float*,
                         Steinberg::uint32, float, Steinberg::uint32) {}

    void SetArpPlayhead(int step);

private:
    void CreateLayout();
    
private:
    Steinberg::Vst::EditController* m_controller{ nullptr };
    std::unique_ptr<AssetManager> m_assetManager;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<GridLayout> m_layout;
    std::unique_ptr<Toolbar> m_toolbar;
    std::unique_ptr<ArpGrid> m_arpGrid;

    int m_width{ 0 };
    int m_height{ 0 };
};

} // namespace oscilleon::gui
