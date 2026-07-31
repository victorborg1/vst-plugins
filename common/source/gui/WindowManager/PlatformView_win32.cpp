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
        hwnd = CreateWindowExA(
            0,
            "STATIC",
            "",
            WS_CHILD |
            WS_VISIBLE |
            WS_CLIPSIBLINGS |
            WS_CLIPCHILDREN,

            0,
            0,
            100,
            100,

            hwndParent,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        if (!hwnd)
            return false;


        SetWindowLongPtr(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(this)
        );

        oldWndProc =
            reinterpret_cast<WNDPROC>(
                SetWindowLongPtr(
                    hwnd,
                    GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(WndProcStatic)
                )
            );

        SetTimer(hwnd, 1, 5, nullptr);

        hdc = GetDC(hwnd);
        if (!hdc) return false;



        PIXELFORMATDESCRIPTOR pfd{};

        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags =
            PFD_DRAW_TO_WINDOW |
            PFD_SUPPORT_OPENGL |
            PFD_DOUBLEBUFFER;

        pfd.iPixelType =
            PFD_TYPE_RGBA;

        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;


        int pf = ChoosePixelFormat(hdc, &pfd);

        if (!pf ||
            !SetPixelFormat(hdc, pf, &pfd))
            return false;



        hglrc = wglCreateContext(hdc);
        if (!hglrc) return false;

        wglMakeCurrent(
            hdc,
            hglrc
        );

        if (!gladLoadGL())
            throw std::runtime_error(
                "gladLoadGL failed"
            );

        return true;
    }



    void detach() override
    {

        if(hwnd) KillTimer(hwnd,1);

        wglMakeCurrent(
            nullptr,
            nullptr
        );

        if(hglrc) {
            wglDeleteContext(hglrc);
            hglrc=nullptr;
        }

        if(hdc) {
            ReleaseDC(hwnd,hdc);
            hdc=nullptr;
        }

        if(hwnd) {
            DestroyWindow(hwnd);
            hwnd=nullptr;
        }
    }



    void makeCurrent() override {
        wglMakeCurrent(
            hdc,
            hglrc
        );
    }



    void swapBuffers() override {
        SwapBuffers(hdc);
    }



    void resize(int w, int h) override {
        SetWindowPos(
            hwnd,
            nullptr,
            0,
            0,
            w,
            h,
            SWP_NOZORDER |
            SWP_NOACTIVATE
        );

        makeCurrent();

        glViewport(
            0,
            0,
            w,
            h
        );
    }



    void* getNativeHandle() override {
        return hwnd;
    }



    void setListener(
        PlatformViewListener* l) override {
        listener = l;
    }



private:

    static LRESULT CALLBACK WndProcStatic(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam)
    {

        auto* self =
            reinterpret_cast<Win32View*>(
                GetWindowLongPtr(
                    hwnd,
                    GWLP_USERDATA
                )
            );


        if(self)
            return self->WndProc(
                hwnd,
                msg,
                wParam,
                lParam
            );


        return DefWindowProc(
            hwnd,
            msg,
            wParam,
            lParam
        );
    }



    LRESULT WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam)
    {

        switch(msg)
        {


        case WM_LBUTTONDOWN:

            if(listener)
                listener->onMouseDown(
                    LOWORD(lParam),
                    HIWORD(lParam)
                );

            return 0;

        case WM_LBUTTONUP:

            if(listener)
                listener->onMouseUp(
                    LOWORD(lParam),
                    HIWORD(lParam)
                );

            return 0;

        case WM_MOUSEMOVE:

            if(listener)
            {
                listener->onMouseMove(
                    LOWORD(lParam),
                    HIWORD(lParam),
                    wParam & MK_LBUTTON,
                    wParam & MK_RBUTTON
                );
            }

            return 0;

        case WM_RBUTTONDOWN:

            if(listener)
                listener->onMouseDown(
                    LOWORD(lParam),
                    HIWORD(lParam)
                );

            return 0;

        case WM_RBUTTONUP:

            if(listener)
                listener->onMouseUp(
                    LOWORD(lParam),
                    HIWORD(lParam)
                );

            return 0;

        case WM_CAPTURECHANGED:

            if(listener)
                listener->onCancelDrag();

            return 0;

        case WM_TIMER:

            if(listener)
                listener->onUpdate(
                    0.005f
                );

            return 0;

        case WM_ERASEBKGND:
            return 1;


        }

        return CallWindowProc(
            oldWndProc,
            hwnd,
            msg,
            wParam,
            lParam
        );
    }



private:

    HWND hwnd = nullptr;
    HWND hwndParent = nullptr;
    HDC hdc = nullptr;
    HGLRC hglrc = nullptr;
    WNDPROC oldWndProc = nullptr;
    PlatformViewListener* listener = nullptr;

};



PlatformView* CreatePlatformView()
{
    return new Win32View();
}


}
