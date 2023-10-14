#include "GLFW/glfw3.h"
#include <iostream>
#include <variant>
#define SK_GANESH
#define SK_GL

#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/gl/GrGLAssembleInterface.h"
#include "include/gpu/gl/GrGLInterface.h"

#include "include/codec/SkCodec.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkData.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkImage.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkPath.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTextBlob.h"
#include "include/effects/SkGradientShader.h"

//#define GL_FRAMEBUFFER_SRGB 0x8DB9
//#define GL_SRGB8_ALPHA8 0x8C43

GrDirectContext *sContext = nullptr;
SkSurface *sSurface = nullptr;

void error_callback(int error, const char *description) {
    fputs(description, stderr);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void init_skia(int w, int h) {
    auto interface1 = GrGLMakeNativeInterface();

    sContext = GrDirectContext::MakeGL(interface1).release();

    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = 0;// assume default framebuffer
    // We are always using OpenGL, and we use RGBA8 internal format for both RGBA and BGRA configs in OpenGL.
    //(replace line below with this one to enable correct color spaces) framebufferInfo.fFormat = GL_SRGB8_ALPHA8;
    framebufferInfo.fFormat = GL_RGBA8;

    SkColorType colorType = kRGBA_8888_SkColorType;
    GrBackendRenderTarget backendRenderTarget = GrBackendRenderTarget(w, h,
                                                                      0,// sample count
                                                                      0,// stencil bits
                                                                      framebufferInfo);

    sSurface = SkSurface::MakeFromBackendRenderTarget(sContext, backendRenderTarget, kBottomLeft_GrSurfaceOrigin, colorType, SkColorSpace::MakeSRGB(), nullptr).release();
    if (sSurface == nullptr) abort();
}

void cleanup_skia() {
    delete sSurface;
    delete sContext;
}

const int kWidth = 1032;
const int kHeight = 670;

struct LyricDynamicWord {
    std::string word;
    float start;
    float end;
};

struct LyricLine {
    float start{};
    float end{};

    bool isDynamic{};
    std::variant<
            std::vector<LyricDynamicWord>,
            std::string>
            lyric;
    std::optional<std::string> translation;
};

struct DynamicLyricWordRenderer {
    virtual float renderLyricWord(SkCanvas *canvas,
                                  float relativeTime,
                                  const LyricDynamicWord &word,
                                  float x, float y,
                                  const SkFont &font, SkPaint &paint1) const = 0;
    [[nodiscard]] virtual float measureLyricWord(const LyricDynamicWord &word, const SkFont &font) const = 0;
};

struct DynamicLyricWordRendererNormal : public DynamicLyricWordRenderer {
    float renderLyricWord(
            SkCanvas *canvas,
            float relativeTime,// relative to the lyric line
            const LyricDynamicWord &word,
            float x, float y,
            const SkFont &font,
            SkPaint &paint1) const override {

        paint1.setAntiAlias(true);
        //  background light
        //        paint1.setColor(SkColorSetARGB(90, 183, 201, 217));
        //        paint1.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.f, 2.f));

        // draw background text
        //        canvas->drawString(word.word.c_str(), x, y, font, paint1);

        // paint with foreground color
        paint1.setMaskFilter(nullptr);
        paint1.setColor(SkColorSetARGB(90, 255, 255, 255));
        paint1.setBlendMode(SkBlendMode::kLighten);

        canvas->drawString(word.word.c_str(), x, y, font, paint1);

        paint1.setBlendMode(SkBlendMode::kPlus);
        // paint dynamic word
        relativeTime = std::clamp(relativeTime - word.start, 0.f, word.end - word.start);
        float progress = (relativeTime) / (word.end - word.start);
        const auto textWidth = font.measureText(word.word.c_str(), word.word.size(), SkTextEncoding::kUTF8);
        // set mask texture
        SkPoint pts[2] = {{x, 0},
                          {x + textWidth, 0}};
        SkColor colors[2] = {SK_ColorWHITE, SK_ColorTRANSPARENT};
        const auto translate = SkMatrix::Translate(textWidth * (progress * 2 - 1), 0);
        const auto shader = SkGradientShader::MakeLinear(pts,
                                                         colors,
                                                         nullptr, 2, SkTileMode::kClamp, 0, &translate);

        paint1.setShader(shader);
        canvas->drawString(word.word.c_str(), x, y, font, paint1);
        return font.measureText(word.word.c_str(), word.word.size(), SkTextEncoding::kUTF8);
    }

    [[nodiscard]] float measureLyricWord(const LyricDynamicWord &word, const SkFont &font) const override {
        return font.measureText(word.word.c_str(), word.word.size(), SkTextEncoding::kUTF8);
    }
};


float renderLyricLine(
        SkCanvas *canvas,
        float relativeTime,
        const LyricLine &line,
        float x, float y,
        float maxWidth,
        const SkFont &font,
        float blur = 0.f) {
    static const float marginVertical = 10.f;
    static const float marginHorizontal = 10.f;
    static const auto renderer = DynamicLyricWordRendererNormal();
    if (line.isDynamic) {
        float currentX = x;
        float currentY = y;
        for (const auto &word: std::get<0>(line.lyric)) {
            float wordWidth = renderer.measureLyricWord(word, font);
            if (currentX + wordWidth > maxWidth) {
                currentX = x;
                currentY += font.getSize() + marginVertical;
            }
            SkPaint paint1;
            paint1.setAntiAlias(true);
            if (blur > 0.f) {
                paint1.setColor(SkColorSetARGB(90, 183, 201, 217));
                paint1.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blur, blur));
            }

            renderer.renderLyricWord(canvas, relativeTime - line.start, word, currentX, currentY, font, paint1);
            currentX += wordWidth + marginHorizontal;
        }

        return currentY - y;
    } else {
        SkPaint paint;
        canvas->drawString(std::get<1>(line.lyric).c_str(), x, y, font, paint);
    }
}


int main() {
    GLFWwindow *window;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    glfwWindowHint(GLFW_ALPHA_BITS, 0);
    glfwWindowHint(GLFW_DEPTH_BITS, 0);
    //glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);

    window = glfwCreateWindow(kWidth, kHeight, "Simple example1", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    glfwSwapInterval(0);

    init_skia(kWidth, kHeight);

    glfwSetKeyCallback(window, key_callback);

    // Draw to the surface via its SkCanvas.
    SkCanvas *canvas = sSurface->getCanvas();// We don't manage this pointer's lifetime.

    float t = 0.f;
    // calc framerate
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    int lastFPS = 0;

    sk_sp<SkFontMgr> fontMgr = SkFontMgr::RefDefault();

    const auto fontFamily = fontMgr->matchFamily("Noto Sans SC");
    const auto typeface = sk_sp(fontFamily->matchStyle(
            SkFontStyle::Bold()));

    // load ./bg.png
    auto data = SkData::MakeFromFileName("bg.png");
    auto pic = SkImage::MakeFromEncoded(data);
    while (!glfwWindowShouldClose(window)) {
        // Measure fps
        double currentTime = glfwGetTime();
        nbFrames++;
        if (currentTime - lastTime >= 1.0) {
            lastFPS = nbFrames;
            nbFrames = 0;
            lastTime = currentTime;
        }

        glfwPollEvents();

        SkPaint paint;
        paint.setColor(SK_ColorWHITE);
        canvas->drawPaint(paint);
        paint.setColor(SK_ColorWHITE);
        canvas->drawImage(
                pic, 0, 0);

        canvas->drawTextBlob(
                SkTextBlob::MakeFromString(
                        std::format("FPS: {} Time: {:.2f}", lastFPS, t).c_str(), SkFont(typeface, 20)),
                10, 30, paint);


        const std::vector<LyricLine> lines{LyricLine{
                                                   .start = 0.f,
                                                   .end = 35.f,
                                                   .isDynamic = true,
                                                   .lyric = std::vector<LyricDynamicWord>({LyricDynamicWord{
                                                                                                   .word = "1213123123123",
                                                                                                   .start = 0.f,
                                                                                                   .end = 5.f},
                                                                                           LyricDynamicWord{
                                                                                                   .word = "1213",
                                                                                                   .start = 6.f,
                                                                                                   .end = 15.f},
                                                                                           LyricDynamicWord{
                                                                                                   .word = "12133123",
                                                                                                   .start = 20.f,
                                                                                                   .end = 35.f}}),
                                                   .translation = std::nullopt},
                                           LyricLine{
                                                   .start = 0.f,
                                                   .end = 35.f,
                                                   .isDynamic = true,
                                                   .lyric = std::vector<LyricDynamicWord>({LyricDynamicWord{
                                                                                                   .word = "23rd2ed",
                                                                                                   .start = 0.f,
                                                                                                   .end = 5.f},
                                                                                           LyricDynamicWord{
                                                                                                   .word = "1213",
                                                                                                   .start = 6.f,
                                                                                                   .end = 15.f},
                                                                                           LyricDynamicWord{
                                                                                                   .word = "12133123",
                                                                                                   .start = 20.f,
                                                                                                   .end = 35.f}}),
                                                   .translation = std::nullopt}};
        // draw lyric
        float offsetY = 500;

        int focusedLineNum = lines.size() - 1;
        for (int i = 0; i < lines.size() - 1; i++) {
            const auto &line = lines[i];
            const auto &nextLine = lines[i + 1];
            if (line.start <= t && line.end >= t || nextLine.start >= t) {
                focusedLineNum = i;
                break;
            }
        }

        float topY = offsetY;
        for (int i = focusedLineNum; i >= 0 && topY > 0; i--) {
            const auto &line = lines[i];
            topY -= renderLyricLine(canvas,
                                    t, line,
                                    10, topY,
                                    kWidth - 20,
                                    SkFont(typeface, 30), 2.f);
        }

        sContext->flush();

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            t += 0.04f;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            t -= 0.04f;

        glfwSwapBuffers(window);
    }

    cleanup_skia();

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
