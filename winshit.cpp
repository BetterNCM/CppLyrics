#include <windows.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "LyricParser.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <thread>

extern std::shared_ptr<std::vector<LyricLine>> _lines_ref;
extern std::atomic<float> currentTimeExt;

int initCppLyrics();
int main() {
    std::thread([]() {
        // sleep 1000ms
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        {
            const auto lyricFile = std::ifstream("../lyric.txt");
            std::stringstream buffer;
            buffer << lyricFile.rdbuf();
            const auto lyricStr = buffer.str();
            *_lines_ref = LyricParser::parse(lyricStr);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        {
            const auto lyricFile = std::ifstream("../lyric2.txt");
            std::stringstream buffer;
            buffer << lyricFile.rdbuf();
            const auto lyricStr = buffer.str();
            *_lines_ref = LyricParser::parse(lyricStr);
            currentTimeExt.exchange(0.f);
        }
    }).detach();
    initCppLyrics();
}

void processWindow(GLFWwindow *win) {
    HWND hwnd = glfwGetWin32Window(win);

    // win10 acrylic effect


    // set window to frameless and parent to ncm win

    // frameless
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_CAPTION);
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_THICKFRAME);
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_SYSMENU);
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_MINIMIZEBOX);
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_MAXIMIZEBOX);
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
    // parent
    HWND parent = FindWindowW(L"OrpheusBrowserHost", nullptr);

    SetParent(hwnd, parent);
    // topmost and lefttop
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);
}