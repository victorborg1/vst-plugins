#include "PlatformView.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glad/glad.h>
#include <GL/glx.h>
#include <stdexcept>
#include <cstring>

namespace Steinberg::oscilleon::gui {

class LinuxView : public PlatformView {
public:

    bool attach(void* parent) override
    {
        if (!parent) {
            printf("parent is null\n");
            return false;
        }
        
        parentWindow = static_cast<Window>(reinterpret_cast<uintptr_t>(parent));
        printf("parentWindow = %lu\n", parentWindow);

        display = XOpenDisplay(nullptr);
        if (!display) {
            printf("XOpenDisplay() failed\n");
            return false;
        }
        printf("XOpenDisplay() OK\n");

        int screen = DefaultScreen(display);


        int attributes[] =
        {
            GLX_X_RENDERABLE, True,
            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
            GLX_RENDER_TYPE, GLX_RGBA_BIT,

            GLX_DOUBLEBUFFER, True,

            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_ALPHA_SIZE, 8,

            GLX_DEPTH_SIZE, 24,

            None
        };

        int glxMajor = 0;
        int glxMinor = 0;
        if (!glXQueryVersion(display, &glxMajor, &glxMinor))
        {
            printf("No GLX support\n");
            return false;
        }
        printf("GLX version %d.%d\n", glxMajor, glxMinor);


        int fbCount = 0;

        GLXFBConfig* configs =
            glXChooseFBConfig(
                display,
                screen,
                attributes,
                &fbCount
            );
        printf("glXChooseFBConfig count = %d\n", fbCount);

        if (!configs || fbCount == 0)
        {
            printf("glXChooseFBConfig() failed, closing display");
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }


        GLXFBConfig fbConfig = configs[0];


        XFree(configs);



        XVisualInfo* visual =
            glXGetVisualFromFBConfig(
                display,
                fbConfig
            );
        printf("visual = %p\n", visual);


        if (!visual)
        {
            printf("glXGetVisualFromFBConfig() failed, closing display");
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }



        XSetWindowAttributes swa{};
        swa.colormap =
            XCreateColormap(
                display,
                RootWindow(display, visual->screen),
                visual->visual,
                AllocNone
            );

        swa.event_mask =
            ExposureMask |
            ButtonPressMask |
            ButtonReleaseMask |
            PointerMotionMask;



        window =
            XCreateWindow(
                display,
                parentWindow,
                0,
                0,
                100,
                100,
                0,
                visual->depth,
                InputOutput,
                visual->visual,
                CWColormap | CWEventMask,
                &swa
            );

        printf("created child window = %lu\n", window);

        if (!window)
        {
            printf("XCreateWindow() failed, closing display");
            XFree(visual);
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }



        XMapWindow(display, window);
        XFlush(display);



        // Create OpenGL context

        context =
            glXCreateNewContext(
                display,
                fbConfig,
                GLX_RGBA_TYPE,
                nullptr,
                True
            );

        printf("GLX context = %p\n", context);

        if (!context)
        {
            printf("glXCreateNewContext() failed, destroying window and closing display");
            XDestroyWindow(display, window);
            XFree(visual);
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }



        glXMakeCurrent(
            display,
            window,
            context
        );


        printf("loading glad...\n");
        if (!gladLoadGL())
        {
            throw std::runtime_error(
                "gladLoadGL() failed"
            );
        }



        XFree(visual);

        return true;
    }



    void detach() override {
        if (!display)
            return;


        glXMakeCurrent(
            display,
            None,
            nullptr
        );


        if (context)
        {
            glXDestroyContext(
                display,
                context
            );

            context = nullptr;
        }


        if (window)
        {
            XDestroyWindow(
                display,
                window
            );

            window = 0;
        }


        XCloseDisplay(display);
        display = nullptr;
    }

    void processEvents() {
        if (!display) return;

        while (XPending(display))
        {
            XEvent event;
            XNextEvent(display, &event);

            switch(event.type)
            {
            case ButtonPress:
            {
                if (!listener)
                    break;

                bool right =
                    event.xbutton.button == Button3;

                listener->onMouseDown(
                    (float)event.xbutton.x,
                    (float)event.xbutton.y,
                    right
                );
                break;
            }

            case ButtonRelease:
            {
                if (!listener)
                    break;

                bool right =
                    event.xbutton.button == Button3;

                listener->onMouseUp(
                    (float)event.xbutton.x,
                    (float)event.xbutton.y,
                    right
                );
                break;
            }


            case MotionNotify:
            {
                if (!listener)
                    break;

                bool left =
                    event.xmotion.state & Button1Mask;

                bool right =
                    event.xmotion.state & Button3Mask;


                listener->onMouseMove(
                    (float)event.xmotion.x,
                    (float)event.xmotion.y,
                    left,
                    right
                );

                break;
            }


            case DestroyNotify:
            {
                if (listener)
                    listener->onCancelDrag();

                break;
            }
            }
        }
    }

    void makeCurrent() override
    {
        if (!display || !context) return;
        processEvents();
        glXMakeCurrent(
            display,
            window,
            context
        );
        
    }



    void swapBuffers() override
    {
        if (display && window)
        {
            glXSwapBuffers(
                display,
                window
            );
        }
    }



    void resize(
        int w,
        int h
    ) override
    {
        if (!display || !window)
            return;


        XResizeWindow(
            display,
            window,
            w,
            h
        );


        glXMakeCurrent(
            display,
            window,
            context
        );


        glViewport(
            0,
            0,
            w,
            h
        );


        XFlush(display);
    }



    void* getNativeHandle() override
    {
        return reinterpret_cast<void*>(
            window
        );
    }

    void setListener(PlatformViewListener* l) override {
        listener = l;
    }

private:

    Display* display = nullptr;
    Window parentWindow = 0;
    Window window = 0;
    GLXContext context = nullptr;
    PlatformViewListener* listener = nullptr;
};



PlatformView* CreatePlatformView() {
    return new LinuxView();
}


} // namespace Steinberg::oscilleon::gui
