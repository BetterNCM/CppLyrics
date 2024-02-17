#define NOMINMAX
#include <Windows.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "../data/DataSource.h"
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

#include "../backend/CppLyricsGLFWVulkanWindow.h"

int main() {
    CppLyricsGLFWVulkanWindow::initGLFW();
    CppLyricsGLFWVulkanWindow::initDawn();

    // set self to high priority
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    const auto lyricFile = std::ifstream("../lyric2.txt");
    std::stringstream buffer;
    buffer << lyricFile.rdbuf();
    const auto lyricStr = buffer.str();

    const auto dataSource = new DataSource();
    dataSource->setLyrics(lyricStr);
    dataSource->setSongInfo("Hard Time", "Seinabo Sey");
    dataSource->setSongColor(std::array<float, 3>{0, 52, 77}, std::array<float, 3>{4, 54, 56});
    dataSource->setSongCover(SkImages::DeferredFromEncodedData(SkData::MakeFromFileName("../cover.jpg")));
    dataSource->setPaused(false);

    const auto image = SkImages::DeferredFromEncodedData(SkData::MakeFromFileName("../cover.jpg"));

    //    std::list<CppLyricsGLFWVulkanWindow> windows{};
    //    for (int i = 0; i < 1; i++)
    //        windows.emplace_back(dataSource);

    auto win = CppLyricsGLFWVulkanWindow(dataSource);
    win.initWindow();
    while (win.render())
        glfwPollEvents();

    //    for (auto &win: windows) {
    //        std::thread([&]() {
    //            win.initWindow();
    //            while (win.render())
    //                glfwPollEvents();
    //        }).detach();
    //    }

    while (1) {
        //            glfwPollEvents();
        Sleep(10000);
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