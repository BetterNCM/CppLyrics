cmake_minimum_required(VERSION 3.26)
project(core)
#// use MTd
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
#// use MT
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(demo main.cpp
        winshit.cpp
        LyricParser.cpp
        LyricParser.h
        Lyric.h
        pch.h
        Utils.cpp
        Utils.h
        render/DynamicLyricWordRenderer.h
        render/DynamicLyricWordRendererNormal.h
        external/utf8.cpp
        external/utf8.h
        render/RenderTextWrap.h
)
target_compile_options(demo PUBLIC /MT)
target_link_directories(demo PUBLIC external/skia/out/Release-x86 external/glfw32/lib-vc2019/)
target_link_libraries(demo PUBLIC skia opengl32 glfw3_mt)
target_include_directories(demo PUBLIC external/skia/include external/skia external/glfw64/include)


add_library(bncm-plugin SHARED main.cpp
        bncm.cpp
        LyricParser.cpp
        LyricParser.h
        Lyric.h
        pch.h
        Utils.cpp
        Utils.h
        render/DynamicLyricWordRenderer.h
        render/DynamicLyricWordRendererNormal.h
        external/utf8.cpp
        external/utf8.h
        render/RenderTextWrap.h
)
target_compile_options(bncm-plugin PUBLIC /MT)
target_link_directories(bncm-plugin PUBLIC external/skia/out/Release-x86 external/glfw32/lib-vc2019/)
target_link_libraries(bncm-plugin PUBLIC skia opengl32 glfw3_mt)
target_include_directories(bncm-plugin PUBLIC external/skia/include external/skia external/glfw64/include
        J:\\Projects\\BetterNCM\\src J:\\Projects\\BetterNCM\\src/3rd/libcef)