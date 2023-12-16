#define NATIVE_PLUGIN_CPP_EXTENSIONS
#define NOMINMAX
#include "../data/DataSource.h"
#include "BetterNCMNativePlugin.h"
#include "GLFW/glfw3.h"
#include "core/SkData.h"
#include "core/SkImage.h"
#include <thread>

#include <array>
#include <fstream>
#include <memory>
#include <sstream>
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "../backend/CppLyricsGLFWWindow.h"

#include "dwmapi.h"
#pragma comment(lib, "dwmapi.lib")

extern "C" {
__declspec(dllexport) int BetterNCMPluginMain(BetterNCMNativePlugin::PluginAPI *api) {
    static CppLyricsGLFWWindow *win = nullptr;
    static DataSource dataSource;

    api->addNativeAPI(
            new NativeAPIType[0]{}, 0, "cpplyrics.init", +[](void **args) -> char * {
                std::thread([&]() {
                    CppLyricsGLFWWindow::initGLFW();
                    win = new CppLyricsGLFWWindow(&dataSource);
                    while (win->render()) {
                        glfwPollEvents();
                    }
                }).detach();
                return nullptr;
            });
    api->addNativeAPI(
            new NativeAPIType[1]{
                    NativeAPIType::String,
            },
            1, "cpplyrics.set_lyrics", +[](void **args) -> char * {
                auto lyrics = std::string(static_cast<char *>(args[0]));
                dataSource.setLyrics(lyrics);
                return nullptr;
            });

    api->addNativeAPI(
            new NativeAPIType[2]{
                    NativeAPIType::Double,
                    NativeAPIType::Boolean},
            2, "cpplyrics.set_time", +[](void **args) -> char * {
                double time = **(double **) args;
                dataSource.setCurrentTime(time);
                return nullptr;
            });

    api->addNativeAPI(
            new NativeAPIType[2]{
                    NativeAPIType::String,
                    NativeAPIType::String},
            2, "cpplyrics.set_song_info", +[](void **args) -> char * {
                auto name = std::string(static_cast<char *>(args[0]));
                auto artist = std::string(static_cast<char *>(args[1]));
                dataSource.setSongInfo(std::move(name), std::move(artist));
                return nullptr;
            });

    api->addNativeAPI(
            new NativeAPIType[6]{
                    NativeAPIType::Int,
                    NativeAPIType::Int,
                    NativeAPIType::Int,
                    NativeAPIType::Int,
                    NativeAPIType::Int,
                    NativeAPIType::Int},
            6, "cpplyrics.set_song_color", +[](void **args) -> char * {
                auto color1 = new std::array<float, 3>();
                auto color2 = new std::array<float, 3>();
                for (int i = 0; i < 3; i++) {
                    (*color1)[i] = **(int **) (args + i);
                    (*color2)[i] = **(int **) (args + i + 3);
                }
                dataSource.setSongColor(*color1, *color2);
                return nullptr;
            });

    api->addNativeAPI(
            new NativeAPIType[1]{NativeAPIType::String},
            1, "cpplyrics.set_song_cover", +[](void **args) -> char * {
                const auto data = SkData::MakeFromFileName(static_cast<char *>(args[0]));
                auto pic = SkImage::MakeFromEncoded(data);
                dataSource.setSongCover(pic);
                return nullptr;
            });

    return 0;
}
}


void processWindow(GLFWwindow *win) {
    HWND hwnd = glfwGetWin32Window(win);

    // win10 acrylic effect

    // set window to frameless and parent to ncm win

    // dwm set corner
    MARGINS margins = {-1};
    DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &margins, sizeof(margins));
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
    // mica
    DWM_BLURBEHIND bb = {
            .dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION,
            .fEnable = TRUE,
            .hRgnBlur = CreateRectRgn(0, 0, -1, -1),
    };
    DwmEnableBlurBehindWindow(hwnd, &bb);
    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_HOSTBACKDROPBRUSH, &value, sizeof(value));
    DWM_SYSTEMBACKDROP_TYPE backdrop_type = DWMSBT_MAINWINDOW;
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop_type, sizeof(backdrop_type));
    // use dark theme
    DWMWINDOWATTRIBUTE attribute = DWMWA_USE_IMMERSIVE_DARK_MODE;
    BOOL useImmersiveDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, attribute, &useImmersiveDarkMode, sizeof(useImmersiveDarkMode));

    // parent
    //    HWND parent = FindWindowW(L"OrpheusBrowserHost", nullptr);
    //
    //    SetParent(hwnd, parent);
    // topmost and lefttop
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);
}
