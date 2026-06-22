#pragma once
#include <windows.h>
#include <glad/glad.h>

// old shit remove later.


typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);

namespace oscilleon::gui {

class WindowManager {
public:
    WindowManager() = default;
    WindowManager(HWND hwnd);
    ~WindowManager();

    void MakeCurrent();
    void SwapBuffers();
    void Resize(int w, int h);

private:
    HWND  m_hwnd  = nullptr;
    HDC   m_hdc   = nullptr;
    HGLRC m_hglrc = nullptr;

    PFNWGLSWAPINTERVALEXTPROC m_wglSwapIntervalEXT = nullptr;
};

} // namespace oscilleon::gui