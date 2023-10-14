cmake_minimum_required(VERSION 3.26)
project(core)
#// use MTd
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
#// use MT
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_STANDARD 23)

# use skia .lib
add_executable(core main.cpp
        winshit.cpp)
target_compile_options(core PUBLIC /MT)
target_link_directories(core PUBLIC external/skia/out/Release-x64 external/glfw64/lib-static-ucrt/)
target_link_libraries(core PUBLIC skia opengl32 glfw3dll)
# add glfw via cmakelist

target_include_directories(core PUBLIC external/skia/include external/skia external/glfw64/include)