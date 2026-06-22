#pragma once

namespace Steinberg::oscilleon::gui  {

class PlatformView {
public:
    virtual ~PlatformView() = default;
    virtual bool attach(void* parent) = 0;
    virtual void detach() = 0;
    virtual void makeCurrent() = 0;
    virtual void swapBuffers() = 0;
    virtual void resize(int w, int h) = 0;
    virtual void* getNativeHandle() = 0;
};

PlatformView* CreatePlatformView();

} // namespace oscilleon::gui