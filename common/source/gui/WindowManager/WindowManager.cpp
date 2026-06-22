#include "WindowManager.h"
#include <stdexcept>

namespace oscilleon::gui {

WindowManager::WindowManager(HWND hwnd)
    : m_hwnd(hwnd)
{
    if (!m_hwnd) throw std::runtime_error("WindowManager: Invalid HWND");

    m_hdc = GetDC(m_hwnd);
    if (!m_hdc) throw std::runtime_error("WindowManager: GetDC() failed");

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int format = ChoosePixelFormat(m_hdc, &pfd);
    if (format == 0 || !SetPixelFormat(m_hdc, format, &pfd))
        throw std::runtime_error("WindowManager: SetPixelFormat() failed");

    m_hglrc = wglCreateContext(m_hdc);
    if (!m_hglrc)
        throw std::runtime_error("WindowManager: wglCreateContext() failed");

    wglMakeCurrent(m_hdc, m_hglrc);

    m_wglSwapIntervalEXT =
        (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (m_wglSwapIntervalEXT)
        m_wglSwapIntervalEXT(1);
}

WindowManager::~WindowManager() {
    if (wglGetCurrentContext() == m_hglrc)
        wglMakeCurrent(nullptr, nullptr);

    if (m_hglrc) {
        wglDeleteContext(m_hglrc);
        m_hglrc = nullptr;
    }
    if (m_hdc && m_hwnd) {
        ReleaseDC(m_hwnd, m_hdc);
        m_hdc = nullptr;
    }
}

void WindowManager::MakeCurrent() {
    if (m_hdc && m_hglrc)
        wglMakeCurrent(m_hdc, m_hglrc);
}

void WindowManager::SwapBuffers() {
    if (m_hdc)
        ::SwapBuffers(m_hdc);
}

void WindowManager::Resize(int w, int h) {
    MakeCurrent();
    glViewport(0, 0, w, h);
}

} // namespace oscilleon::gui