#define NATIVE_PLUGIN_CPP_EXTENSIONS
#define NOMINMAX
#include "J:\Projects\BetterNCM\src\BetterNCMNativePlugin.h"
#include "LyricParser.h"
#include "core/SkData.h"
#include "core/SkImage.h"
#include <thread>

#include <array>
#include <fstream>
#include <memory>
#include <sstream>

extern std::shared_ptr<std::vector<LyricLine>> _lines_ref;
extern std::atomic<float> currentTimeExt;
extern std::atomic<bool> isPaused;
extern std::atomic<std::shared_ptr<std::string>> songName;
extern std::atomic<std::shared_ptr<std::string>> songArtist;
extern std::atomic<std::array<float, 3> *> songColor1;
extern std::atomic<std::array<float, 3> *> songColor2;
extern sk_sp<SkImage> songCover;
int initCppLyrics();

extern "C" {
__declspec(dllexport) int BetterNCMPluginMain(BetterNCMNativePlugin::PluginAPI *api) {
    api->addNativeAPI(
            new NativeAPIType[0]{}, 0, "cpplyrics.init", +[](void **args) -> char * {
                std::thread([]() {
                    initCppLyrics();
                }).detach();
                return nullptr;
            });
    api->addNativeAPI(
            new NativeAPIType[1]{
                    NativeAPIType::String,
            },
            1, "cpplyrics.set_lyrics", +[](void **args) -> char * {
                auto lyrics = std::string(static_cast<char *>(args[0]));
                *_lines_ref = LyricParser::parse(lyrics);
                return nullptr;
            });

    api->addNativeAPI(
            new NativeAPIType[2]{
                    NativeAPIType::Double,
                    NativeAPIType::Boolean},
            2, "cpplyrics.set_time", +[](void **args) -> char * {
                double time = **(double **) args;
                currentTimeExt.exchange(time);
                isPaused.exchange(**(bool **) (args + 1));
                return nullptr;
            });

    api->addNativeAPI(
            new NativeAPIType[2]{
                    NativeAPIType::String,
                    NativeAPIType::String},
            2, "cpplyrics.set_song_info", +[](void **args) -> char * {
                auto name = std::string(static_cast<char *>(args[0]));
                auto artist = std::string(static_cast<char *>(args[1]));
                songName.exchange(std::make_shared<std::string>(name));
                songArtist.exchange(std::make_shared<std::string>(artist));
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
                songColor1.exchange(color1);
                songColor2.exchange(color2);
                return nullptr;
            });

    api->addNativeAPI(
            new NativeAPIType[1]{NativeAPIType::String},
            1, "cpplyrics.set_song_cover", +[](void **args) -> char * {
                const auto data = SkData::MakeFromFileName(static_cast<char *>(args[0]));
                const auto pic = SkImage::MakeFromEncoded(data);
                songCover = pic;
                return nullptr;
            });

    return 0;
}
}
