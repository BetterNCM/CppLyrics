#define NOMINMAX
#include <Windows.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "../parser/LyricParser.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "core/SkData.h"
#include "core/SkImage.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <thread>

#include "dwmapi.h"
#pragma comment(lib, "dwmapi.lib")

#include "../backend/CppLyricsGLFWWindow.h"

int main() {
    // set self to high priority
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    const auto lyricFile = std::ifstream("../lyric2.txt");
    std::stringstream buffer;
    buffer << lyricFile.rdbuf();
    const auto lyricStr = buffer.str();
    const auto lines = LyricParser::parse(lyricStr);

    CppLyricsGLFWWindow::initGLFW();
    const auto image = SkImage::MakeFromEncoded(SkData::MakeFromFileName("../cover.jpg"));
    std::vector<CppLyricsGLFWWindow> windows{5};
    for (auto &win: windows) {
        win.cppLyrics.lines = lines;
        win.cppLyrics.songName = "Hard Time";
        win.cppLyrics.songArtist = "Seinabo Sey";
        win.cppLyrics.songColor1 = std::array<float, 3>{0, 52, 77};
        win.cppLyrics.songColor2 = std::array<float, 3>{4, 54, 56};
        win.cppLyrics.songCover = SkImage::MakeFromEncoded(SkData::MakeFromFileName("../cover.jpg"));
    }

    for (auto &win: windows) {
        std::thread([&]() {
            while (win.render())
                ;
        }).detach();
    }

    while (1) {
        glfwPollEvents();
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