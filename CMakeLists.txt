cmake_minimum_required(VERSION 3.26)
project(core)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

set(CMAKE_CXX_STANDARD 23)

add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)
set(GLFW_VULKAN_STATIC ON)

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("external/dawn" EXCLUDE_FROM_ALL)

add_executable(demo main.h
        entry/demo.cpp
        data/LyricParser.cpp
        data/LyricParser.h
        Lyric.h
        pch.h
        utils/Utils.cpp
        utils/Utils.h
        render/DynamicLyricWordRenderer.h
        render/DynamicLyricWordRendererNormal.h
        external/utf8.cpp
        external/utf8.h
        render/RenderTextWrap.h
        main.cpp
        main.cpp
        backend/CppLyricsGLFWWindow.cpp
        backend/CppLyricsGLFWWindow.h
        data/DataSource.cpp
        data/DataSource.h
        backend/CppLyricsGLFWVulkanWindow.h
        vk-bootstrap/VkBootstrap.h
        vk-bootstrap/VkBootstrap.cpp
        vk-bootstrap/VkBootstrapDispatch.h
)
target_link_directories(demo PUBLIC external/skia/prebuilt external/glfw3/cmake-build-release/src external/vulkan/Lib32 external/dawn/prebuilt/x86/)
target_link_libraries(demo PUBLIC dxguid webgpu_glfw skia glfw)
target_include_directories(demo PUBLIC external/skia/include external/skia external/glfw3/include external/vulkan/include external/dawn/include)


add_library(bncm-plugin SHARED main.h
        entry/bncm.cpp
        data/LyricParser.cpp
        data/LyricParser.h
        Lyric.h
        pch.h
        utils/Utils.cpp
        utils/Utils.h
        render/DynamicLyricWordRenderer.h
        render/DynamicLyricWordRendererNormal.h
        external/utf8.cpp
        external/utf8.h
        render/RenderTextWrap.h
        main.cpp
        main.cpp
        backend/CppLyricsGLFWWindow.cpp
        backend/CppLyricsGLFWWindow.h
        data/DataSource.cpp
        data/DataSource.h
        backend/CppLyricsGLFWVulkanWindow.h
        vk-bootstrap/VkBootstrap.h
        vk-bootstrap/VkBootstrap.cpp
        vk-bootstrap/VkBootstrapDispatch.h
)
target_link_directories(bncm-plugin PUBLIC external/skia/prebuilt external/glfw3/cmake-build-release/src external/vulkan/Lib32)
target_link_libraries(bncm-plugin PUBLIC skia)
target_include_directories(bncm-plugin PUBLIC external/skia/include external/skia external/glfw3/include
        J:\\Projects\\BetterNCM\\src J:\\Projects\\BetterNCM\\src/3rd/libcef external/vulkan/include)