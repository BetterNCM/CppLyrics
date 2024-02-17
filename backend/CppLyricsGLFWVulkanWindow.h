#pragma once

#include "../main.h"
#include "../pch.h"

#define SK_DAWN
#include "dawn/dawn_proc.h"

#include "include/core/SkPicture.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkSize.h"
#include "include/gpu/graphite/BackendTexture.h"
#include "include/gpu/graphite/Context.h"
#include "include/gpu/graphite/ContextOptions.h"
#include "include/gpu/graphite/Surface.h"
#include "include/gpu/graphite/dawn/DawnBackendContext.h"
#include "include/gpu/graphite/dawn/DawnTypes.h"
#include "include/gpu/graphite/dawn/DawnUtils.h"
#if defined(_WIN32)
#include <Windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include "../cmake-build-release/external/dawn/gen/include/dawn/dawn_proc_table.h"
#include "GLFW/glfw3native.h"
#include "dawn/native/DawnNative.h"
#endif

#include "include/gpu/graphite/dawn/DawnBackendContext.h"

wgpu::Instance instance;
//TODO: move implementations to a separate file
class CppLyricsGLFWVulkanWindow {
    wgpu::Surface surface;
    wgpu::Device device;
    wgpu::SwapChain swapChain;
    std::unique_ptr<skgpu::graphite::Context> skiaCtx;
    std::unique_ptr<skgpu::graphite::Recorder> recorder;
    sk_sp<SkColorSpace> colorSpace;
    SkSurfaceProps props;
    static void error_callback(int error, const char *description) {
        std::cout << error << " " << description;
    }
    static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
    }

    void SetupSwapChain(wgpu::Surface surface) {
        wgpu::SwapChainDescriptor scDesc{
                .usage = wgpu::TextureUsage::RenderAttachment |
                         wgpu::TextureUsage::TextureBinding |
                         wgpu::TextureUsage::CopySrc |
                         wgpu::TextureUsage::CopyDst,
                .format = wgpu::TextureFormat::BGRA8Unorm,
                .width = kWidth,
                .height = kHeight,
                .presentMode = wgpu::PresentMode::Immediate};
        swapChain = device.CreateSwapChain(surface, &scDesc);
    }

    void GetDevice(std::function<void(wgpu::Device)> callback) {
        instance.RequestAdapter(
                nullptr,
                [](WGPURequestAdapterStatus status, WGPUAdapter cAdapter,
                   const char *message, void *userdata) {
                    if (status != WGPURequestAdapterStatus_Success) {
                        exit(0);
                    }
                    wgpu::Adapter adapter = wgpu::Adapter::Acquire(cAdapter);
                    wgpu::DeviceDescriptor deviceDescriptor;

                    std::vector<wgpu::FeatureName> requiredFeatures;
                    requiredFeatures.push_back(wgpu::FeatureName::SurfaceCapabilities);

                    deviceDescriptor.requiredFeatures = requiredFeatures.data();
                    deviceDescriptor.requiredFeatureCount = requiredFeatures.size();

                    adapter.RequestDevice(
                            &deviceDescriptor,
                            [](WGPURequestDeviceStatus status, WGPUDevice cDevice,
                               const char *message, void *userdata) {
                                wgpu::Device device = wgpu::Device::Acquire(cDevice);
                                (*reinterpret_cast<std::function<void(wgpu::Device)> *>(userdata))(device);
                            },
                            userdata);
                },
                reinterpret_cast<void *>(&callback));
    }

    void init_dawn_for_win() {
        std::promise<void> devicePromise;

        std::async(std::launch::async, [this, &devicePromise]() {
            GetDevice([&](wgpu::Device device) {
                std::cout << "Device created\n";
                this->device = device;
                devicePromise.set_value();
            });
        });

        devicePromise.get_future().wait();
        device.SetLoggingCallback([](WGPULoggingType type, char const *message, void *) {
            std::cout << message << std::endl;
        },
                                  nullptr);

        device.SetUncapturedErrorCallback([](WGPUErrorType type, char const *message, void *) {
            std::cerr << message << std::endl;
        },
                                          nullptr);

        SetupSwapChain(surface);
    }

    void init_skia() {
        skiaCtx = skgpu::graphite::ContextFactory::MakeDawn(
                skgpu::graphite::DawnBackendContext(device, device.GetQueue()),
                skgpu::graphite::ContextOptions());

        recorder = skiaCtx->makeRecorder();
        colorSpace = SkColorSpace::MakeSRGB();
        props = SkSurfaceProps(0, kBGR_H_SkPixelGeometry);
    }

    sk_sp<SkSurface> getBackbufferSurface() {
        auto texture = swapChain.GetCurrentTexture();
        auto backendTexture = skgpu::graphite::BackendTexture(texture.Get());
        return SkSurfaces::WrapBackendTexture(
                recorder.get(), backendTexture, kBGRA_8888_SkColorType, colorSpace, &props);
    }

    uint32_t kWidth = 400;
    uint32_t kHeight = 600;
    bool limitFPS = true;
    bool topMost = false;

public:
    CppLyrics cppLyrics;
    static void initGLFW() {
        glfwInit();
        glfwSetErrorCallback(error_callback);
    }

    static void initDawn() {
        DawnProcTable backendProcs = dawn::native::GetProcs();
        dawnProcSetProcs(&backendProcs);

        wgpu::InstanceDescriptor desc = {};
        desc.nextInChain = nullptr;
        instance = wgpu::CreateInstance(&desc);
    }

    explicit CppLyricsGLFWVulkanWindow(const CppLyricsGLFWVulkanWindow &cppLyricsGLFWWindow) = delete;
    ~CppLyricsGLFWVulkanWindow() {
        if (window) {
            glfwDestroyWindow(window);
        }
    }
    explicit CppLyricsGLFWVulkanWindow(DataSource *const &dataSource) : cppLyrics(dataSource) {
        static GLFWwindow *firstWin = nullptr;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        //        glfwWindowHint(GLFW_DECORATED, 0);
        //        glfwWindowHint(GLFW_RESIZABLE, 1);
        //        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(kWidth, kHeight, "CppLyrics", nullptr, firstWin);
        firstWin = window;

        surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);

        init_dawn_for_win();
    }
    void initWindow() {

        if (!window) {
            glfwTerminate();
            std::cout << "glfwCreateWindow failed";
            exit(EXIT_FAILURE);
        }
        glfwSetKeyCallback(window, key_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSetWindowUserPointer(window, this);
        cppLyrics.kHeight = kHeight;
        cppLyrics.kWidth = kWidth;
        glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int width, int height) {
            const auto cppLyricsWin = ((CppLyricsGLFWVulkanWindow *) glfwGetWindowUserPointer(window));
            cppLyricsWin->resize(width, height);
        });
    }
    double lastAnimeTime = -1;
    double lastFPSUpdateTime = -1;
    int frameCount = 0;
    void resize(int width, int height) {
        kWidth = width;
        kHeight = height;
        cppLyrics.kHeight = height;
        cppLyrics.kWidth = width;
        SetupSwapChain(surface);
    }
    bool skia_inited = false;
    bool render() {
        if (!skia_inited) {
            skia_inited = true;
            init_skia();
            SetupSwapChain(surface);
        }

        if (glfwWindowShouldClose(window))
            return false;

        const auto surf = getBackbufferSurface();

        //        SkPictureRecorder record;
        //        SkCanvas *canvas = record.beginRecording(surf->imageInfo().bounds().width(),
        //                                                 surf->imageInfo().bounds().height());
        auto canvas = surf->getCanvas();

        auto paint = SkPaint();
        //        SkPoint pts[] = {SkPoint{0, 0}, SkPoint{(float) kWidth, 0}};
        //        SkColor colors[] = {SK_ColorRED, SK_ColorBLUE};
        //        paint.setShader(
        //                SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
        //                                             SkTileMode::kClamp));
        paint.setColor(SK_ColorRED);
        canvas->drawRect(
                SkRect::MakeXYWH(0, 0, surf->imageInfo().bounds().width(),
                                 surf->imageInfo().bounds().height()),
                paint);
                
        //        std::cout << "Width: " << surf->imageInfo().bounds().width() << ", Height: " << surf->imageInfo().bounds().height() << "\n";

        //        canvas->drawRect(
        //                SkRect::MakeXYWH(0, 0, surf->imageInfo().bounds().width(),
        //                                 surf->imageInfo().bounds().height()),
        //                paint);

        //        cppLyrics.render(canvas, surf.get());

        //        const auto data = record.finishRecordingAsPicture()->serialize();
        // write to file
        //        std::ofstream file("test.skp", std::ios::binary);
        //        file.write((char *) data->data(), data->size());
        //        file.close();
        //        std::terminate();

        //        cppLyrics.render(canvas, surf.get());

        std::unique_ptr<skgpu::graphite::Recording> recording = recorder->snap();
        if (recording) {
            skgpu::graphite::InsertRecordingInfo info;
            info.fRecording = recording.get();
            skiaCtx->insertRecording(info);
            skiaCtx->submit(skgpu::graphite::SyncToCpu::kNo);
        }

        swapChain.Present();

        doLogic();
        return true;
    }
    void doLogic() {
        const auto currentTime = glfwGetTime();
        if (lastFPSUpdateTime < 0)
            lastFPSUpdateTime = currentTime;
        frameCount++;
        if (currentTime - lastFPSUpdateTime > 1) {
            cppLyrics.lastFPS = frameCount;
            frameCount = 0;
            lastFPSUpdateTime = currentTime;
        }
        if (lastAnimeTime < 0)
            lastAnimeTime = currentTime;

        const auto deltaTime = (currentTime - lastAnimeTime) * 1000;
        lastAnimeTime = currentTime;
        cppLyrics.animate(deltaTime);
        processKeyEvts(deltaTime);
    }
    GLFWwindow *window = nullptr;

    void processKeyEvts(float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cppLyrics.t += 7.f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cppLyrics.t -= 7.f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
            cppLyrics.t = 0.f;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
#if defined(_WIN32)
            HWND hwnd = glfwGetWin32Window(window);
            SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
#endif
        }

        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            cppLyrics.useFluentBg = !cppLyrics.useFluentBg;
            while (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
                glfwPollEvents();
        }

        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
            cppLyrics.useFontBlur = !cppLyrics.useFontBlur;
            while (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
                glfwPollEvents();
        }

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            cppLyrics.useTextResize = !cppLyrics.useTextResize;
            while (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
                glfwPollEvents();
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cppLyrics.useSingleLine = !cppLyrics.useSingleLine;
            while (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                glfwPollEvents();
        }

        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            limitFPS = !limitFPS;
            while (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
                glfwPollEvents();
            if (limitFPS) {
                glfwSwapInterval(1);
            } else {
                glfwSwapInterval(0);
            }
        }

        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !topMost) {
            // taskbar mode
            cppLyrics.useFluentBg = false;
            cppLyrics.useFontBlur = false;
            cppLyrics.useTextResize = false;
            cppLyrics.useSingleLine = true;
            cppLyrics.showTips = false;
            cppLyrics.showSongInfo = false;
            cppLyrics.useRoundBorder = true;
            cppLyrics.minTextSize = 12.f;
            cppLyrics.maxTextSize = 18.f;
            cppLyrics.subLyricsMarginTop = 4.f;
            cppLyrics.marginBottomLyrics = 6.f;
            cppLyrics.wordMargin = 5.f;
            resize(200, 44);
            topMost = true;
#if defined(_WIN32)
            HWND hwnd = glfwGetWin32Window(window);

            std::thread([hwnd, this]() {
                while (topMost) {
                    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }).detach();
#endif
            while (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
                glfwPollEvents();
        }
    }
};
