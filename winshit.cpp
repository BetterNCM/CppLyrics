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

extern std::shared_ptr<std::vector<LyricLine>> _lines_ref;
extern std::atomic<float> currentTimeExt;
extern std::atomic<bool> isPaused;
extern std::atomic<std::shared_ptr<std::string>> songName;
extern std::atomic<std::shared_ptr<std::string>> songArtist;
extern std::atomic<std::array<float, 3> *> songColor1;
extern std::atomic<std::array<float, 3> *> songColor2;
extern sk_sp<SkImage> songCover;

int initCppLyrics();
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

            songName.exchange(std::make_shared<std::string>("song name877878787787887878787887"));
            songArtist.exchange(std::make_shared<std::string>("song artist"));
            songColor1.exchange(new std::array<float, 3>{0, 52, 77});
            songColor2.exchange(new std::array<float, 3>{4, 54, 56});
            songCover = SkImage::MakeFromEncoded(SkData::MakeFromFileName("../cover.jpg"));
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