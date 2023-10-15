#define NATIVE_PLUGIN_CPP_EXTENSIONS
#include "J:\Projects\BetterNCM\src\BetterNCMNativePlugin.h"
#include "LyricParser.h"
#include <thread>

#include <fstream>
#include <memory>
#include <sstream>

extern std::shared_ptr<std::vector<LyricLine>> _lines_ref;
extern std::atomic<float> currentTimeExt;
extern std::atomic<bool> isPaused;
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

    return 0;
}
}
