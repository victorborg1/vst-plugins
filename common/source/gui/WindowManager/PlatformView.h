#pragma once

namespace Steinberg::oscilleon::gui  {

class PlatformViewListener {
public:
    virtual ~PlatformViewListener() = default;
    virtual void onMouseDown(float x, float y, bool rightButton) = 0;
    virtual void onMouseUp(float x, float y, bool rightButton) = 0;
    virtual void onMouseMove(float x, float y, bool leftDown, bool rightDown) = 0;
    virtual void onCancelDrag() = 0;
    virtual void onUpdate(float dt) = 0;
    virtual void onRenderRequested() = 0;
};

class PlatformView {
public:
    virtual ~PlatformView() = default;
    virtual bool attach(void* parent) = 0;
    virtual void detach() = 0;
    virtual void makeCurrent() = 0;
    virtual void swapBuffers() = 0;
    virtual void resize(int w, int h) = 0;
    virtual void* getNativeHandle() = 0;
    virtual void setListener(PlatformViewListener* listener) = 0;
};

PlatformView* CreatePlatformView();

} // namespace Steinberg::oscilleon::gui
