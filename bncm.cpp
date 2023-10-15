#define NATIVE_PLUGIN_CPP_EXTENSIONS
#include "J:\Projects\BetterNCM\src\BetterNCMNativePlugin.h"

int initCppLyrics();

extern "C" {
__declspec(dllexport) int BetterNCMPluginMain(BetterNCMNativePlugin::PluginAPI *api) {
    api->addNativeAPI(
            new NativeAPIType[0]{}, 0, "cpplyrics.init", +[](void **args) -> char * {
                return std::to_string(initCppLyrics()).data();
            });

    return 0;
}
}
