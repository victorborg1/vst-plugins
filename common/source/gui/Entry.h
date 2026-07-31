#pragma once

#include "pluginterfaces/vst/ivstplugview.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "WindowManager/PlatformView.h"

#include <memory>
#include <atomic>
#include <stdexcept>

namespace Steinberg {
namespace oscilleon {
namespace gui {

template<typename EditorType, int WindowWidth, int WindowHeight>
class Entry
    : public Steinberg::IPlugView
    , public PlatformViewListener
{
private:
    using tresult   = Steinberg::tresult;
    using uint32    = Steinberg::uint32;
    using TBool     = Steinberg::TBool;
    using ViewRect  = Steinberg::ViewRect;
    using IPlugFrame = Steinberg::IPlugFrame;
    using FIDString = Steinberg::FIDString;
    using TUID      = Steinberg::TUID;
    using char16    = Steinberg::char16;
    using int16     = Steinberg::int16;

public:
    explicit Entry(Vst::EditController* controller)
        : m_controller(controller)
    {
        if (m_controller) m_controller->addRef();
    }

    ~Entry() {
        CleanUp();
        if (m_controller) m_controller->release();
    }

    void remember() { addRef(); }
    void forget()   { release(); }

    void updateUI(Vst::ParamID id, Vst::ParamValue valueNormalized) {
        if (m_editor)
            m_editor->UpdateParameter(id, static_cast<float>(valueNormalized));
    }

    // exchange
    void setWaveformData(const float* L, const float* R,
                        uint32 count, float bufferSizeSec, uint32 writePos)
    {
        if (m_editor)
            m_editor->SetWaveformData(L, R, count, bufferSizeSec, writePos);
    }

    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override {
        if (!obj) return kInvalidArgument;
        if (FUnknownPrivate::iidEqual(_iid, IPlugView::iid)) {
            *obj = static_cast<IPlugView*>(this); addRef(); return kResultOk;
        }
        if (FUnknownPrivate::iidEqual(_iid, FUnknown::iid)) {
            *obj = static_cast<FUnknown*>(this); addRef(); return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }

    uint32 PLUGIN_API addRef()  override { return ++m_refCount; }
    
    uint32 PLUGIN_API release() override {
        auto r = --m_refCount;
        if (r == 0) delete this;
        return r;
    }

    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override {
#if defined(_WIN32)
        return FIDStringsEqual(type, kPlatformTypeHWND)
            ? kResultTrue
            : kResultFalse;

#elif defined(__linux__)
        return FIDStringsEqual(type, kPlatformTypeX11EmbedWindowID)
            ? kResultTrue
            : kResultFalse;

#elif defined(__APPLE__)
        return FIDStringsEqual(type, kPlatformTypeNSView)
            ? kResultTrue
            : kResultFalse;
#else
        return kResultFalse;
#endif
    }

    tresult PLUGIN_API attached(void* parent, FIDString) override {
        if (!parent) return kResultFalse;

        m_platformView = CreatePlatformView();
        if (!m_platformView || !m_platformView->attach(parent))
            throw std::runtime_error("PlatformView::attach() failed");
        m_platformView->setListener(this);

        
        m_editor = std::make_unique<EditorType>(m_controller);
        m_editor->Init(m_width, m_height);
        m_platformView->resize(m_width, m_height);
        
        if (m_frame) {
            ViewRect vr{ 0, 0, m_width, m_height };
            m_frame->resizeView(this, &vr);
        }
        Render();
        return kResultOk;
    }


    tresult PLUGIN_API removed() override {
        CleanUp();
        return kResultOk;
    }

    tresult PLUGIN_API onSize(ViewRect* newSize) override {
        if (!newSize || !m_platformView) return kResultFalse;
        int w = newSize->getWidth();
        int h = newSize->getHeight();
        if (w == m_width && h == m_height) return kResultOk;
        m_width = w;
        m_height = h;
        m_platformView->resize(w, h);   // also updates glViewport
        if (m_editor) m_editor->Resize(w, h);
        Render();
        return kResultOk;
    }

    tresult PLUGIN_API getSize(ViewRect* size) override {
        if (!size) return kResultFalse;
        size->left = 0;
        size->top = 0;
        size->right = m_width;
        size->bottom = m_height;
        return kResultOk;
    }

    tresult PLUGIN_API checkSizeConstraint(ViewRect* rect) override {
        if (!rect) return kResultFalse;
        if (rect->getWidth() < WindowWidth)   rect->right  = rect->left + WindowWidth;
        if (rect->getHeight() < WindowHeight) rect->bottom = rect->top  + WindowHeight;
        return kResultOk;
    }

    tresult PLUGIN_API canResize()                             override { return kResultTrue;  }
    tresult PLUGIN_API onFocus(TBool)                          override { return kResultOk;    }
    tresult PLUGIN_API setFrame(IPlugFrame* frame)             override { m_frame = frame; return kResultOk; }
    tresult PLUGIN_API onKeyDown(char16, int16, int16)         override { return kResultOk;    }
    tresult PLUGIN_API onKeyUp(char16, int16, int16)           override { return kResultOk;    }
    tresult PLUGIN_API onWheel(float)                          override { return kResultOk;    }
    
    // platformviewlistener

    void onMouseDown(float x,float y,bool rightButton) override {
        if (!m_editor)
            return;

        if(rightButton)
            m_editor->OnRightMouseDown(x,y);
        else
            m_editor->OnMouseDown(x,y);

        Render();
    }


    void onMouseUp(float x,float y,bool rightButton) override {
        if (!m_editor)
            return;

        if(rightButton)
            m_editor->OnRightMouseUp(x,y);
        else
            m_editor->OnMouseUp(x,y);

        Render();
    }

    void onMouseMove(float x, float y, bool leftDown, bool rightDown) override {
        if (!m_editor)
            return;

        if (leftDown)
        {
            m_editor->OnMouseMove(x, y);
            Render();
        }
        else if (rightDown)
        {
            m_editor->OnRightMouseMove(x, y);
            Render();
        }
    }


    void onCancelDrag() override {
        if (m_editor)
            m_editor->CancelDrag();

        Render();
    }


    void onUpdate(float dt) override {
        if (m_editor)
        {
            m_editor->UpdateWidgets(dt);
            Render();
        }
    }


    void onRenderRequested() override {
        Render();
    }


private:

    void Render() {
        if (!m_platformView || !m_editor) return;
        m_platformView->makeCurrent();
        m_editor->Render();
        m_platformView->swapBuffers();
    }


    void CleanUp() {

        if (m_platformView) {
            m_platformView->setListener(nullptr);
            m_platformView->makeCurrent();
            m_editor.reset();

            m_platformView->detach();
            delete m_platformView;
            m_platformView = nullptr;
        }
    }

private:
    int m_width { WindowWidth  };
    int m_height{ WindowHeight };

    PlatformView*                   m_platformView = nullptr;
    Vst::EditController*            m_controller   = nullptr;
    IPlugFrame*                     m_frame        = nullptr;
    std::atomic<uint32>             m_refCount{ 1 };
    std::unique_ptr<EditorType>     m_editor;
};

} // namespace gui
} // namespace oscilleon
} // namespace Steinberg
