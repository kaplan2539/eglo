/**
 * eglo -- Minimalistic X11/EGL abstraction for Pocket C.H.I.P
 * Copyright (c) 2016, Thomas Perl <m@thp.io>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 **/


#include "eglo.h"

#include <cstdio>
#include <cstdlib>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <EGL/egl.h>

#define FAIL(...) do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)

namespace {

class X11Window {
public:
    X11Window(int width, int height);
    ~X11Window();

    int process_event(EgloEvent *event);
    void swap();

    int get_width() { return width; }
    int get_height() { return height; }

protected:
    int width;
    int height;
    Display *dpy;
    Window win;
};

X11Window::X11Window(int width, int height)
    : width(width)
    , height(height)
    , dpy()
    , win()
{
   dpy = XOpenDisplay(":0");
   if (dpy == nullptr) {
       FAIL("cannot connect to X server");
   }

   Window root = DefaultRootWindow(dpy);

   XSetWindowAttributes swa;
   swa.event_mask = (ExposureMask | PointerMotionMask | KeyPressMask | ButtonPressMask |
           ButtonReleaseMask | Button1MotionMask);

   win = XCreateWindow(dpy, root, 0, 0, width, height, 0, CopyFromParent,
           InputOutput, CopyFromParent, CWEventMask, &swa);

   XSetWindowAttributes xattr;
   xattr.override_redirect = False;
   XChangeWindowAttributes(dpy, win, CWOverrideRedirect, &xattr);

   Atom atom = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", True);
   XChangeProperty(dpy, win, XInternAtom(dpy, "_NET_WM_STATE", True),
           XA_ATOM, 32, PropModeReplace, (unsigned char *)&atom, 1);

   XWMHints hints;
   hints.input = True;
   hints.flags = InputHint;
   XSetWMHints(dpy, win, &hints);

   XMapWindow(dpy, win);
   XStoreName(dpy, win, "EGLO");
   XMoveResizeWindow(dpy, win, 0, 0, width, height);
}

X11Window::~X11Window()
{
   XDestroyWindow(dpy, win);
   //XCloseDisplay(dpy);
}

int
X11Window::process_event(EgloEvent *event)
{
    if (XPending(dpy)) {
        XEvent xev;
        XNextEvent(dpy, &xev);

        if (xev.type == MotionNotify) {
            event->type = EGLO_MOUSE_MOTION;
            event->mouse.x = xev.xmotion.x;
            event->mouse.y = xev.xmotion.y;
            return 1;
        } else if (xev.type == ButtonPress) {
            event->type = EGLO_MOUSE_DOWN;
            event->mouse.x = xev.xbutton.x;
            event->mouse.y = xev.xbutton.y;
            return 1;
        } else if (xev.type == ButtonRelease) {
            event->type = EGLO_MOUSE_UP;
            event->mouse.x = xev.xbutton.x;
            event->mouse.y = xev.xbutton.y;
            return 1;
        } else if (xev.type == KeyPress) {
            event->type = EGLO_KEY_DOWN;
            // TODO
            return 1;
        } else if (xev.type == KeyRelease) {
            event->type = EGLO_KEY_UP;
            // TODO
            return 1;
        } else {
            printf("Unhandled: %d\n", xev.type);
        }
    }

    return 0;
}

void
X11Window::swap()
{
    XForceScreenSaver(dpy, ScreenSaverReset);
}

class EGLWindow : public X11Window {
public:
    EGLWindow(int width, int height, int gles_version, const char *version);
    ~EGLWindow();

    void swap();

protected:
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    const char *version;
};

EGLWindow::EGLWindow(int width, int height, int gles_version, const char *version)
    : X11Window(width, height)
    , egl_display()
    , egl_context()
    , egl_surface()
    , version(version)
{
   egl_display = eglGetDisplay((EGLNativeDisplayType)dpy);
   if (egl_display == EGL_NO_DISPLAY) {
      FAIL("Got no EGL display.");
   }

   if (!eglInitialize(egl_display, nullptr, nullptr)) {
      FAIL("Unable to initialize EGL: 0x%08x", eglGetError());
   }

   EGLint attr[] = {
      EGL_RENDERABLE_TYPE, (gles_version == 2) ? EGL_OPENGL_ES2_BIT : EGL_OPENGL_ES_BIT,
      EGL_NONE
   };

   EGLConfig ecfg;
   EGLint num_config;

   if (!eglChooseConfig(egl_display, attr, &ecfg, 1, &num_config)) {
      FAIL("Failed to choose config (eglError: 0x%08x)", eglGetError());
   }

   if (num_config != 1) {
      FAIL("Didn't get exactly one config, but %d", num_config);
   }

   egl_surface = eglCreateWindowSurface(egl_display, ecfg, win, nullptr);
   if (egl_surface == EGL_NO_SURFACE) {
      FAIL("Unable to create EGL surface (eglError: 0x%08x)", eglGetError());
   }

   EGLint ctxattr[] = {
      EGL_CONTEXT_CLIENT_VERSION, gles_version,
      EGL_NONE
   };

   egl_context = eglCreateContext(egl_display, ecfg, EGL_NO_CONTEXT, ctxattr);
   if (egl_context == EGL_NO_CONTEXT) {
      FAIL("Unable to create EGL context (eglError: 0x%08x)", eglGetError());
   }

   eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
}

EGLWindow::~EGLWindow()
{
   eglDestroyContext(egl_display, egl_context);
   eglDestroySurface(egl_display, egl_surface);
   eglTerminate(egl_display);
}

void
EGLWindow::swap()
{
    eglSwapBuffers(egl_display, egl_surface);
    X11Window::swap();
}


const char *eglo_version_string =
"@(#) eglo 0.0.1 (2016-11-24) -- Copyright (c) 2016, Thomas Perl";

constexpr int DEFAULT_WIDTH = 480 + 1;
constexpr int DEFAULT_HEIGHT = 272;

EGLWindow *g_eglo_window = nullptr;

};

#ifdef __cplusplus
extern "C" {
#endif

int eglo_init(int *width, int *height, int gles_version)
{
    eglo_quit();

    setenv("DISPLAY", ":0", 0);

    g_eglo_window = new EGLWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, gles_version, eglo_version_string);

    if (width != nullptr) {
        *width = g_eglo_window->get_width();
    }

    if (height != nullptr) {
        *height = g_eglo_window->get_height();
    }

    return 0;
}

int eglo_next_event(EgloEvent *event)
{
    if (g_eglo_window != nullptr) {
        return g_eglo_window->process_event(event);
    }

    return 0;
}

void eglo_swap_buffers()
{
    if (g_eglo_window != nullptr) {
        g_eglo_window->swap();
    }
}

void eglo_quit()
{
    if (g_eglo_window != nullptr) {
        delete g_eglo_window, g_eglo_window = nullptr;
    }
}

#ifdef __cplusplus
};
#endif
