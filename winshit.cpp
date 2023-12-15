#define NOMINMAX
#include <windows.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "LyricParser.h"
#include "core/SkData.h"
#include "core/SkImage.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <thread>

#include "dwmapi.h"
#pragma comment(lib, "dwmapi.lib")

#include "CppLyricsGLFWWindow.h"

extern std::shared_ptr<std::vector<LyricLine>> _lines_ref;
extern std::atomic<float> currentTimeExt;
extern std::atomic<bool> isPaused;
extern std::atomic<std::shared_ptr<std::string>> songName;
extern std::atomic<std::shared_ptr<std::string>> songArtist;
extern std::atomic<std::array<float, 3> *> songColor1;
extern std::atomic<std::array<float, 3> *> songColor2;
extern sk_sp<SkImage> songCover;

int main() {
    // set self to high priority
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    std::thread([]() {
        // sleep 1000ms
        //        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        //
        //        {
        //            const auto lyricFile = std::ifstream("../lyric.txt");
        //            std::stringstream buffer;
        //            buffer << lyricFile.rdbuf();
        //            const auto lyricStr = buffer.str();
        //            *_lines_ref = LyricParser::parse(lyricStr);
        //        }
        //        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        {
            const auto lyricFile = std::ifstream("../lyric2.txt");
            std::stringstream buffer;
            buffer << lyricFile.rdbuf();
            const auto lyricStr = buffer.str();
            *_lines_ref = LyricParser::parse(lyricStr);
            currentTimeExt.exchange(0.f);

            songName.exchange(std::make_shared<std::string>("Hard Time"));
            songArtist.exchange(std::make_shared<std::string>("Seinabo Sey"));
            songColor1.exchange(new std::array<float, 3>{0, 52, 77});
            songColor2.exchange(new std::array<float, 3>{4, 54, 56});
            songCover = SkImage::MakeFromEncoded(SkData::MakeFromFileName("../cover.jpg"));
        }
    }).detach();

    CppLyricsGLFWWindow window1;
    CppLyricsGLFWWindow window2;

    while (!glfwWindowShouldClose(window1.window) && !glfwWindowShouldClose(window2.window)) {
        glfwPollEvents();
        window1.render();
        window2.render();
    }
}


void processWindow(GLFWwindow *win) {
    HWND hwnd = glfwGetWin32Window(win);

    // win10 acrylic effect

    // set window to frameless and parent to ncm win

    // dwm set corner
    if (true) {
        MARGINS margins = {-1};
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &margins, sizeof(margins));
        DwmExtendFrameIntoClientArea(hwnd, &margins);
        //        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_CAPTION);
        //        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_THICKFRAME);
        //        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_SYSMENU);
        //        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_MINIMIZEBOX);
        //        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_MAXIMIZEBOX);
        // mica
        DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
        BOOL value = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_HOSTBACKDROPBRUSH, &value, sizeof(value));
        DWM_SYSTEMBACKDROP_TYPE backdrop_type = DWMSBT_MAINWINDOW;
        DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop_type, sizeof(backdrop_type));
        // use dark theme
        DWMWINDOWATTRIBUTE attribute = DWMWA_USE_IMMERSIVE_DARK_MODE;
        BOOL useImmersiveDarkMode = TRUE;
        DwmSetWindowAttribute(hwnd, attribute, &useImmersiveDarkMode, sizeof(useImmersiveDarkMode));
    } else {
    }


    // parent
    //    HWND parent = FindWindowW(L"OrpheusBrowserHost", nullptr);
    //
    //    SetParent(hwnd, parent);
    // topmost and lefttop
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);
}