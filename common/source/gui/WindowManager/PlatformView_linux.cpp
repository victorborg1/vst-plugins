#include "PlatformView.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glad/glad.h>
#include <GL/glx.h>
#include <cstdint>
#include <mutex>

namespace Steinberg::oscilleon::gui {

static int XErrorSwallow(Display*, XErrorEvent*) { return 0; }
static bool EnsureGLLoaded()
{
    return gladLoadGL() != 0;
}

typedef void (*PFNGLXSWAPINTERVALEXTPROC)(Display*, GLXDrawable, int);
typedef int  (*PFNGLXSWAPINTERVALMESAPROC)(unsigned int);

static void DisableSwapThrottling(Display* display, GLXWindow glxWindow) {
    auto swapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)
        glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");
    if (swapIntervalEXT) {
        swapIntervalEXT(display, glxWindow, 0);
        return;
    }
    auto swapIntervalMESA = (PFNGLXSWAPINTERVALMESAPROC)
        glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalMESA");
    if (swapIntervalMESA) swapIntervalMESA(0);
}

class LinuxView : public PlatformView {
public:

    bool attach(void* parent) override
    {
        if (!parent) return false;

        XSetErrorHandler(XErrorSwallow);
        XInitThreads();

        parentWindow = (Window)(uintptr_t)(parent);

        display = XOpenDisplay(nullptr);
        if (!display) return false;

        int screen = DefaultScreen(display);

        int attributes[] = {
            GLX_X_RENDERABLE, True,
            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
            GLX_RENDER_TYPE, GLX_RGBA_BIT,
            GLX_DOUBLEBUFFER, True,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_DEPTH_SIZE, 24,
            None
        };

        int fbCount = 0;
        GLXFBConfig* configs = glXChooseFBConfig(display, screen, attributes, &fbCount);
        if (!configs || fbCount == 0) {
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }

        GLXFBConfig fbConfig = configs[0];
        XFree(configs);

        XVisualInfo* visual = glXGetVisualFromFBConfig(display, fbConfig);
        if (!visual) {
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }

        XSetWindowAttributes swa{};
        swa.colormap = XCreateColormap(display,
                                       RootWindow(display, visual->screen),
                                       visual->visual,
                                       AllocNone);

        swa.event_mask = ExposureMask |
                         ButtonPressMask |
                         ButtonReleaseMask |
                         PointerMotionMask |
                         ButtonMotionMask |
                         StructureNotifyMask;

        window = XCreateWindow(display, parentWindow,
                               0, 0, 100, 100, 0,
                               visual->depth,
                               InputOutput,
                               visual->visual,
                               CWColormap | CWEventMask,
                               &swa);

        if (!window) {
            XFree(visual);
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }

        XMapWindow(display, window);
        XFlush(display);

        glxWindow = glXCreateWindow(display, fbConfig, window, nullptr);
        if (!glxWindow) {
            XFree(visual);
            XDestroyWindow(display, window);
            window = 0;
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }

        context = glXCreateNewContext(display, fbConfig, GLX_RGBA_TYPE, nullptr, True);
        XFree(visual);

        if (!context) {
            glXDestroyWindow(display, glxWindow);
            glxWindow = 0;
            XDestroyWindow(display, window);
            window = 0;
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }

        glXMakeCurrent(display, glxWindow, context);
        DisableSwapThrottling(display, glxWindow);

        if (!EnsureGLLoaded()) {
            glXMakeCurrent(display, None, nullptr);
            glXDestroyContext(display, context);
            context = nullptr;
            glXDestroyWindow(display, glxWindow);
            glxWindow = 0;
            XDestroyWindow(display, window);
            window = 0;
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }

        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glXSwapBuffers(display, glxWindow);
        XSync(display, False);

        return true;
    }

    void startRendering() override {}
    void stopRendering() override {}

    void detach() override
    {
        std::lock_guard<std::mutex> lock(mtx);

        if (display) {
            if (context) {
                glXMakeCurrent(display, None, nullptr);
                glXDestroyContext(display, context);
                context = nullptr;
            }
            if (glxWindow) {
                glXDestroyWindow(display, glxWindow);
                glxWindow = 0;
            }
            if (window) {
                XDestroyWindow(display, window);
                window = 0;
            }
            XCloseDisplay(display);
            display = nullptr;
        }
    }

    void makeCurrent() override {
        std::lock_guard<std::mutex> lock(mtx);
        if (display && context && glxWindow)
            glXMakeCurrent(display, glxWindow, context);
    }

    void swapBuffers() override {
        std::lock_guard<std::mutex> lock(mtx);
        if (display && glxWindow)
            glXSwapBuffers(display, glxWindow);
    }

    void resize(int w, int h) override {
        std::lock_guard<std::mutex> lock(mtx);
        if (display && window) {
            XResizeWindow(display, window, w, h);
            XFlush(display);
        }
    }

    void* getNativeHandle() override {
        std::lock_guard<std::mutex> lock(mtx);
        return (void*)(uintptr_t)window;
    }

    void setListener(PlatformViewListener* l) override {
        std::lock_guard<std::mutex> lock(mtx);
        listener = l;
    }

    int getPollDescriptor() override {
        std::lock_guard<std::mutex> lock(mtx);
        return display ? ConnectionNumber(display) : -1;
    }

    void processEvents() override
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!display || !window || !context) return;

        glXMakeCurrent(display, glxWindow, context);

        while (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);

            switch (event.type) {
            case ButtonPress: {
                if (!listener) break;
                bool right = (event.xbutton.button == Button3);
                XGrabPointer(display, window, True,
                             ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                             GrabModeAsync, GrabModeAsync,
                             None, None, CurrentTime);
                listener->onMouseDown((float)event.xbutton.x,
                                     (float)event.xbutton.y, right);
                break;
            }
            case ButtonRelease: {
                if (!listener) break;
                bool right = (event.xbutton.button == Button3);
                listener->onMouseUp((float)event.xbutton.x,
                                   (float)event.xbutton.y, right);
                XUngrabPointer(display, CurrentTime);
                break;
            }
            case MotionNotify: {
                if (!listener) break;
                bool left  = event.xmotion.state & Button1Mask;
                bool right = event.xmotion.state & Button3Mask;
                if (left || right) {
                    listener->onMouseMove((float)event.xmotion.x,
                                         (float)event.xmotion.y,
                                         left, right);
                }
                break;
            }
            case ConfigureNotify:
                glViewport(0, 0, event.xconfigure.width, event.xconfigure.height);
                break;
            case DestroyNotify:
                if (listener) listener->onCancelDrag();
                break;
            }
        }
    }

private:
    Display* display = nullptr;
    Window parentWindow = 0;
    Window window = 0;
    GLXWindow glxWindow = 0;
    GLXContext context = nullptr;
    PlatformViewListener* listener = nullptr;
    std::mutex mtx;
};

PlatformView* CreatePlatformView() {
    return new LinuxView();
}

} // namespace Steinberg::oscilleon::gui
