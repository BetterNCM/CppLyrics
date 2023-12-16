#pragma once

#include "main.h"
#include "pch.h"

#if defined(_WIN32)
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#endif

class CppLyricsGLFWWindow {
    GrDirectContext *sContext = nullptr;
    SkSurface *sSurface = nullptr;
    GrBackendRenderTarget backendRenderTarget;
    CppLyrics cppLyrics;

    static void error_callback(int error, const char *description) {
        std::cout << error << " " << description;
    }
    static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
    }
    void init_skia(int w, int h) {
        if (!sContext) {
            auto interface1 = GrGLMakeNativeInterface();
            sContext = GrDirectContext::MakeGL(interface1).release();
        }

        GrGLFramebufferInfo framebufferInfo;
        framebufferInfo.fFBOID = 0;// assume default framebuffer
        // We are always using OpenGL, and we use RGBA8 internal format for both RGBA and BGRA configs in OpenGL.
        //(replace line below with this one to enable correct color spaces) framebufferInfo.fFormat = GL_SRGB8_ALPHA8;
        framebufferInfo.fFormat = GL_RGBA8;

        SkColorType colorType = kRGBA_8888_SkColorType;
        auto backendRenderTarget = GrBackendRenderTarget(w, h,
                                                         1,// sample count
                                                         0,// stencil bits
                                                         framebufferInfo);

        sSurface = SkSurface::MakeFromBackendRenderTarget(
                           sContext, backendRenderTarget, kBottomLeft_GrSurfaceOrigin,
                           colorType, SkColorSpace::MakeSRGB(), nullptr)
                           .release();
        if (sSurface == nullptr) abort();
    }

    int kWidth = 400;
    int kHeight = 600;

public:
    // delete copy constructor
    CppLyricsGLFWWindow(const CppLyricsGLFWWindow &cppLyricsGLFWWindow) = delete;
    ~CppLyricsGLFWWindow() {
        if (sContext) {
            sContext->flushAndSubmit(true);
            sContext->releaseResourcesAndAbandonContext();
            delete sContext;
        }
        if (sSurface) {
            delete sSurface;
        }
        if (window) {
            glfwDestroyWindow(window);
        }
    }
    CppLyricsGLFWWindow() {
        static std::atomic<bool> isGLFWInited = false;
        if (!isGLFWInited.exchange(true)) {
            glfwSetErrorCallback(error_callback);
            if (!glfwInit()) {
                std::cout << "glfwInit failed";
                exit(EXIT_FAILURE);
            }
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_DECORATED, 0);
        glfwWindowHint(GLFW_RESIZABLE, 1);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
        window = glfwCreateWindow(kWidth, kHeight, "CppLyrics", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            std::cout << "glfwCreateWindow failed";
            exit(EXIT_FAILURE);
        }
        glfwSetKeyCallback(window, key_callback);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        init_skia(kWidth, kHeight);
        glfwSetKeyCallback(window, key_callback);
        glfwSetWindowUserPointer(window, this);
        cppLyrics.kHeight = kHeight;
        cppLyrics.kWidth = kWidth;
        glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int width, int height) {
            const auto cppLyricsWin = ((CppLyricsGLFWWindow *) glfwGetWindowUserPointer(window));
            cppLyricsWin->resize(width, height);
        });
    }
    double lastAnimeTime = -1;
    double lastFPSUpdateTime = -1;
    int frameCount = 0;
    void resize(int width, int height) {
        kWidth = width;
        kHeight = height;
        cppLyrics.kHeight = height;
        cppLyrics.kWidth = width;
        init_skia(width, height);
    }
    bool render() {
        glfwMakeContextCurrent(window);
        if (glfwWindowShouldClose(window)) {
            return false;
        }
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        auto canvas = sSurface->getCanvas();
        canvas->clear(SK_ColorTRANSPARENT);
        cppLyrics.render(canvas);
        sContext->flush();
        glfwSwapBuffers(window);


        const auto currentTime = glfwGetTime();
        if (lastFPSUpdateTime < 0)
            lastFPSUpdateTime = currentTime;
        frameCount++;
        if (currentTime - lastFPSUpdateTime > 1) {
            cppLyrics.lastFPS = frameCount;
            frameCount = 0;
            lastFPSUpdateTime = currentTime;
        }
        if (lastAnimeTime < 0)
            lastAnimeTime = currentTime;

        const auto deltaTime = (currentTime - lastAnimeTime) * 1000;
        lastAnimeTime = currentTime;
        cppLyrics.animate(deltaTime);
        processKeyEvts(deltaTime);
        return true;
    }
    GLFWwindow *window = nullptr;
    void processKeyEvts(float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cppLyrics.t += 7.f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cppLyrics.t -= 7.f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
            cppLyrics.t = 0.f;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
#if defined(_WIN32)
            HWND hwnd = glfwGetWin32Window(window);
            SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
#endif
        }

        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            cppLyrics.useFluentBg = !cppLyrics.useFluentBg;
            while (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
                glfwPollEvents();
        }

        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
            cppLyrics.useFontBlur = !cppLyrics.useFontBlur;
            while (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
                glfwPollEvents();
        }

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            cppLyrics.useTextResize = !cppLyrics.useTextResize;
            while (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
                glfwPollEvents();
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cppLyrics.useSingleLine = !cppLyrics.useSingleLine;
            while (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                glfwPollEvents();
        }

        //        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        //            cppLyrics.showTips = !cppLyrics.showTips;
        //            while (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        //                glfwPollEvents();
        //        }

        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
            // taskbar mode
            cppLyrics.useFluentBg = false;
            cppLyrics.useFontBlur = false;
            cppLyrics.useTextResize = false;
            cppLyrics.useSingleLine = true;
            cppLyrics.showTips = false;
            cppLyrics.showSongInfo = false;
            cppLyrics.minTextSize = 12.f;
            cppLyrics.maxTextSize = 18.f;
            cppLyrics.subLyricsMarginTop = 4.f;
            cppLyrics.marginBottomLyrics = 6.f;
            cppLyrics.wordMargin = 5.f;
#if defined(_WIN32)
            HWND hwnd = glfwGetWin32Window(window);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#endif
            while (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
                glfwPollEvents();
        }
    }
};
