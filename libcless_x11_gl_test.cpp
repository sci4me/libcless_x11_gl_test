#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

extern "C" void* syscall5(
    void* number,
    void* arg1,
    void* arg2,
    void* arg3,
    void* arg4,
    void* arg5
);

typedef unsigned long int uintptr;
typedef long int intptr;

static intptr write(int fd, void const* data, uintptr nbytes) {
    return (intptr)
        syscall5(
            (void*)1,
            (void*)(intptr)fd,
            (void*)data,
            (void*)nbytes,
            0,
            0
        );
}

uintptr strlen(char const* s) {
    uintptr len = 0;
    while(*s++) len++;
    return len;
}

static void print(char const* str) {
    write(0, (void const*)str, strlen(str));
}

static void println(char const* str) {
    print(str);
    print("\n");
}

static void print_int(int x) {
    char stack[256];
    int sp = 0;

    while(x >= 10) {
        int r = x % 10;
        stack[sp++] = (char)(r + 48);
        x /= 10;
    }
    stack[sp++] = (char)((x % 10) + 48);

    write(0, (void const*)stack, sp);
}

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

int main(int argc, char **argv) {
    Display *dsp = XOpenDisplay(0);

    if(!dsp) {
        print("Failed to open X Display! :(\n");
        return 1;
    }

    GLint glx_major, glx_minor;
    glXQueryVersion(dsp, &glx_major, &glx_minor);
    if(glx_major < 1 || glx_minor < 4) {
        print("GLX version too old! :(\n");
        XCloseDisplay(dsp);
        return 1;
    }

    Window root = DefaultRootWindow(dsp);

    GLint att[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        None
    };

    int fb_count;
    GLXFBConfig *fbcs = glXChooseFBConfig(dsp, DefaultScreen(dsp), att, &fb_count);
    if(fbcs == 0) {
        print("Failed to retrieve framebuffer configs! :(\n");
        XCloseDisplay(dsp);
        return 1;
    }

    // TODO: Pick the best out of fbcs
    XVisualInfo *vi = glXGetVisualFromFBConfig(dsp, fbcs[0]);
    if(!vi) {
        print("Failed to choose XVisualInfo! :(\n");
        XCloseDisplay(dsp);
        return 1;
    }

    Colormap cmap = XCreateColormap(dsp, root, vi->visual, AllocNone);

    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;

    Window win = XCreateWindow(dsp, root, 0, 0, 600, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    if(!win) {
        print("Failed to create window! :(\n");
        // TODO
        return 1;
    }
    
    XFree(vi);

    XStoreName(dsp, win, "libcless_x11_gl_test");

    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddress((GLubyte const*)"glXCreateContextAttribsARB");

    // NOTE: Calling this directly isn't "safe" if we're running with
    // an ancient version of GLX. (I say ancient, but, according to the
    // docs it requires 1.4 which is the version I have. *shrugs*)
    int ctx_att[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 4,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        None
    };
    GLXContext glc = glXCreateContextAttribsARB(dsp, fbcs[0], 0, True, ctx_att);
    XSync(dsp, False);

    // TODO X error handler thing...
    
    Atom atomWmDeleteWindow = XInternAtom(dsp, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dsp, win, &atomWmDeleteWindow, 1);

    if(!glXIsDirect(dsp, glc)) {
        print("GLXContext is indirect! :(\n");

        glXDestroyContext(dsp, glc);
        XDestroyWindow(dsp, win);
        XCloseDisplay(dsp);
        return 1;
    }

    glXMakeCurrent(dsp, win, glc);


    GLint gl_major, gl_minor; 
    glGetIntegerv(GL_MAJOR_VERSION, &gl_major); 
    glGetIntegerv(GL_MINOR_VERSION, &gl_minor);

    if(gl_major < 4 || gl_minor < 4) {
        print("OpenGL version too old! :(\n");

        glXMakeCurrent(dsp, None, 0);
        glXDestroyContext(dsp, glc);
        XDestroyWindow(dsp, win);
        XCloseDisplay(dsp);
        return 1;
    }

    print("GL_MAJOR:                    "); print_int(gl_major); print("\n");
    print("GL_MINOR:                    "); print_int(gl_minor); print("\n");
    print("GL_VENDOR:                   "); println((char const*) glGetString(GL_VENDOR));
    print("GL_RENDERER:                 "); println((char const*) glGetString(GL_RENDERER));
    print("GL_VERSION:                  "); println((char const*) glGetString(GL_VERSION));
    print("GL_SHADING_LANGUAGE_VERSION: "); println((char const*) glGetString(GL_SHADING_LANGUAGE_VERSION));

    XMapRaised(dsp, win);

    bool running = true;
    while(running) {
        XEvent xev;
        XNextEvent(dsp, &xev);

        switch(xev.type) {
            case Expose: {
                XWindowAttributes gwa;
                XGetWindowAttributes(dsp, win, &gwa);

                glViewport(0, 0, gwa.width, gwa.height);
                break;
            }
            case ClientMessage: {
                if(xev.xclient.data.l[0] == atomWmDeleteWindow) {
                    running = false;
                }
                break;
            }
            case KeyPress: {
                break;
            }
        }

        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        // modern gl here! :P
        

        glXSwapBuffers(dsp, win);
    }

    glXMakeCurrent(dsp, None, 0);
    glXDestroyContext(dsp, glc);
    XDestroyWindow(dsp, win);
    XCloseDisplay(dsp);

    print("Shut down successfully! :)\n");

	return 0;
}