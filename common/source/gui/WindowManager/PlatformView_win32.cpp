#include "PlatformView.h"
#include <windows.h>
#include <glad/glad.h>
#include <stdexcept>

namespace Steinberg::oscilleon::gui {

class Win32View : public PlatformView {
public:
    bool attach(void* parent) override {
        if (!parent) return false;
        hwndParent = static_cast<HWND>(parent);

        // Create a child window so we own the surface —
        // never subclass the DAW parent directly.
        hwnd = CreateWindowExA(
            0, "STATIC", "",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 100, 100,
            hwndParent, nullptr, GetModuleHandle(nullptr), nullptr);

        if (!hwnd) return false;

        hdc = GetDC(hwnd);
        if (!hdc) return false;

        PIXELFORMATDESCRIPTOR pfd{};
        pfd.nSize      = sizeof(pfd);
        pfd.nVersion   = 1;
        pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;

        int pf = ChoosePixelFormat(hdc, &pfd);
        if (!pf || !SetPixelFormat(hdc, pf, &pfd)) return false;

        hglrc = wglCreateContext(hdc);
        if (!hglrc) return false;

        wglMakeCurrent(hdc, hglrc);

        if (!gladLoadGL())
            throw std::runtime_error("gladLoadGL() failed");

        return true;
    }

    void detach() override {
        wglMakeCurrent(nullptr, nullptr);
        if (hglrc) { wglDeleteContext(hglrc); hglrc = nullptr; }
        if (hdc)   { ReleaseDC(hwnd, hdc);    hdc   = nullptr; }
        if (hwnd)  { DestroyWindow(hwnd);      hwnd  = nullptr; }
    }

    void makeCurrent() override {
        wglMakeCurrent(hdc, hglrc);
    }

    void swapBuffers() override {
        SwapBuffers(hdc);
    }

    void resize(int w, int h) override {
        SetWindowPos(hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
        wglMakeCurrent(hdc, hglrc);   // viewport change requires active context
        glViewport(0, 0, w, h);
    }

    void* getNativeHandle() override {
        return hwnd;
    }

private:
    HWND  hwnd       = nullptr;
    HWND  hwndParent = nullptr;
    HDC   hdc        = nullptr;
    HGLRC hglrc      = nullptr;
};

PlatformView* CreatePlatformView() {
    return new Win32View();
}

} // namespace oscilleon::gui