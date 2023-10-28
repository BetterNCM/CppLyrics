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
    //    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_THICKFRAME);
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_SYSMENU);
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_MINIMIZEBOX);
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_MAXIMIZEBOX);
    //    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
    // ^ click through

    // dwm set corner
    MARGINS margins = {-1};
    DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &margins, sizeof(margins));
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
    // mica
    DWM_BLURBEHIND bb = {0};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.hRgnBlur = CreateRectRgn(0, 0, -1, -1);
    bb.fEnable = true;
    DwmEnableBlurBehindWindow(hwnd, &bb);
    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_HOSTBACKDROPBRUSH, &value, sizeof(value));
    DWM_SYSTEMBACKDROP_TYPE backdrop_type = DWMSBT_TABBEDWINDOW;
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop_type, sizeof(backdrop_type));
    // use dark theme
    DWMWINDOWATTRIBUTE attribute = DWMWA_USE_IMMERSIVE_DARK_MODE;
    BOOL useImmersiveDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, attribute, &useImmersiveDarkMode, sizeof(useImmersiveDarkMode));

    // hook window message to make it draggable and resizable

#define GET_X_LPARAM(lp) ((int) (short) LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int) (short) HIWORD(lp))

    SetWindowsHookEx(
            WH_CALLWNDPROC, [](int nCode, WPARAM wParam, LPARAM lParam) -> LRESULT {
                if (nCode == HC_ACTION) {
                    auto msg = reinterpret_cast<CWPSTRUCT *>(lParam);
                    if (msg->message == WM_NCHITTEST) {
                        return HTCAPTION;
                        auto x = GET_X_LPARAM(msg->lParam);
                        auto y = GET_Y_LPARAM(msg->lParam);
                        auto rect = RECT{};
                        GetWindowRect(msg->hwnd, &rect);
                        if (x >= rect.left && x <= rect.left + 5 && y >= rect.top && y <= rect.top + 5) {
                            return HTTOPLEFT;
                        } else if (x >= rect.right - 5 && x <= rect.right && y >= rect.top && y <= rect.top + 5) {
                            return HTTOPRIGHT;
                        } else if (x >= rect.left && x <= rect.left + 5 && y >= rect.bottom - 5 && y <= rect.bottom) {
                            return HTBOTTOMLEFT;
                        } else if (x >= rect.right - 5 && x <= rect.right && y >= rect.bottom - 5 && y <= rect.bottom) {
                            return HTBOTTOMRIGHT;
                        } else if (x >= rect.left && x <= rect.left + 5) {
                            return HTLEFT;
                        } else if (x >= rect.right - 5 && x <= rect.right) {
                            return HTRIGHT;
                        } else if (y >= rect.top && y <= rect.top + 5) {
                            return HTTOP;
                        } else if (y >= rect.bottom - 5 && y <= rect.bottom) {
                            return HTBOTTOM;
                        }
                    }
                }
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            },
            nullptr, GetCurrentThreadId());

    // parent
    //    HWND parent = FindWindowW(L"OrpheusBrowserHost", nullptr);
    //
    //    SetParent(hwnd, parent);
    // topmost and lefttop
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);
}