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

int main(int argc, char **argv) {
    Display *dsp = XOpenDisplay(0);

    if(!dsp) {
        print("Failed to open X Display! :(\n");
        return 1;
    }

    Window root = DefaultRootWindow(dsp);

    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo *vi = glXChooseVisual(dsp, 0, att);

    if(!vi) {
        print("Failed to choose XVisualInfo! :(\n");
        return 1;
    }

    Colormap cmap = XCreateColormap(dsp, root, vi->visual, AllocNone);

    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask;

    Window win = XCreateWindow(dsp, root, 0, 0, 600, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

    XMapWindow(dsp, win);
    XStoreName(dsp, win, "libcless_x11_gl_test");

    GLXContext glc = glXCreateContext(dsp, vi, 0, GL_TRUE);
    glXMakeCurrent(dsp, win, glc);

    bool running = true;
    while(running) {
        XEvent xev;
        XNextEvent(dsp, &xev);

        switch(xev.type) {
            case Expose: {
                XWindowAttributes gwa;
                XGetWindowAttributes(dsp, win, &gwa);

                glViewport(0, 0, gwa.width, gwa.height);


                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);


                glColor3f(1.0f, 0.0f, 0.0f);
                glBegin(GL_TRIANGLES);
                    glVertex2f( 0.0f, -1.0f);
                    glVertex2f(-1.0f,  1.0f);
                    glVertex2f( 1.0f,  1.0f);
                glEnd();

                glXSwapBuffers(dsp, win);
                break;
            }
            case KeyPress: {
                running = false;
                break;
            }
        }
    }

    glXMakeCurrent(dsp, None, 0);
    glXDestroyContext(dsp, glc);
    XDestroyWindow(dsp, win);
    XCloseDisplay(dsp);

	return 0;
}