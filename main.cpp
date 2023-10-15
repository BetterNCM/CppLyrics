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

#define GL_FRAMEBUFFER_SRGB 0x8DB9
#define GL_SRGB8_ALPHA8 0x8C43

static const bool enableLowEffect = false;

GrDirectContext *sContext = nullptr;
SkSurface *sSurface = nullptr;

#define drawHorizontalLineAt(y, color)               \
    {                                                \
        SkPaint ___paint;                            \
        ___paint.setColor(SkColors::color);          \
        ___paint.setStrokeWidth(1.f);                \
        canvas->drawLine(0, y, kWidth, y, ___paint); \
    }

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
extern std::vector<LyricLine> lines;
struct LyricRenderDataLine {
    float minHeight;
};
struct LyricRenderData {
    std::vector<LyricRenderDataLine> lines;
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
        relativeTime = std::clamp(relativeTime - word.start, 0.f, word.end - word.start);
        float progress = (relativeTime) / (word.end - word.start);

        y -= (1 - progress) * 4.f - font.getSize();

        paint1.setAntiAlias(true);
        //  background light
        //        paint1.setColor(SkColorSetARGB(90, 183, 201, 217));
        //        paint1.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.f, 2.f));

        // draw background text
        //        canvas->drawString(word.word.c_str(), x, y, font, paint1);

        // paint with foreground color
        //        paint1.setMaskFilter(nullptr);
        paint1.setBlendMode(SkBlendMode::kPlus);
        paint1.setColor(SkColorSetARGB(90, 255, 255, 255));

        canvas->drawString(word.word.c_str(), x, y, font, paint1);

        paint1.setBlendMode(SkBlendMode::kSrcOver);
        // paint dynamic word

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

#define ANIMATED_FLOAT(name, initVal, step) \
    struct name##_t {                       \
        float current = initVal;            \
        float animateState = 1.0f;          \
        float target = initVal;             \
        float stepPerFrame = step;          \
        void animateTo(float target) {      \
            this->target = target;          \
            this->animateState = 0.0f;      \
        }                                   \
        void setTo(float target) {          \
            this->target = target;          \
            this->animateState = 1.0f;      \
            this->current = target;         \
        }                                   \
    } name;

struct LyricRenderContext {
    ANIMATED_FLOAT(offsetY, 0.f, 0.0005f)
    ANIMATED_FLOAT(currentLine, 0.f, 0.05f)
} lyric_ctx;

float renderLyricLine(
        SkCanvas *canvas,
        float relativeTime,
        const LyricLine &line,
        float x, float y,
        float maxWidth,
        const SkFont &font,
        const SkFont &layoutFont,
        float blur = 0.f) {
    static const float marginVertical = 10.f;
    static const float marginHorizontal = 10.f;
    static const auto renderer = DynamicLyricWordRendererNormal();
    if (line.isDynamic) {
        float currentX = x;
        float currentXLayout = x;
        float currentY = y;
        for (const auto &word: std::get<0>(line.lyric)) {
            float wordWidth = renderer.measureLyricWord(word, font);
            float wordWidthLayout = renderer.measureLyricWord(word, layoutFont);
            if (currentXLayout + wordWidthLayout - x > maxWidth) {
                currentX = x;
                currentXLayout = x;
                currentY += font.getSize() + marginVertical;
            }
            SkPaint paint1;
            paint1.setAntiAlias(true);
            if (blur > 0.f) {
                paint1.setColor(SkColorSetARGB(90, 183, 201, 217));
                if (!enableLowEffect) {
                    paint1.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blur, blur));
                }
            }

            renderer.renderLyricWord(canvas, relativeTime - line.start, word, currentX, currentY, font, paint1);
            currentX += wordWidth + marginHorizontal;
            currentXLayout += wordWidthLayout + marginHorizontal;
        }

        return currentY - y + font.getSize() + marginVertical + 20.f;
    } else {
        SkPaint paint;
        canvas->drawString(std::get<1>(line.lyric).c_str(), x, y, font, paint);
    }
}

float estimateLyricLineHeight(
        const LyricLine &line,
        float maxWidth,
        const SkFont &font,
        const SkFont &layoutFont) {
    static const float marginVertical = 10.f;
    static const float marginHorizontal = 10.f;
    static const auto renderer = DynamicLyricWordRendererNormal();

    if (line.isDynamic) {
        float currentX = 0;
        float currentXLayout = 0;
        float currentY = 0;
        for (const auto &word: std::get<0>(line.lyric)) {
            float wordWidth = renderer.measureLyricWord(word, font);
            float wordWidthLayout = renderer.measureLyricWord(word, layoutFont);
            if (currentXLayout + wordWidthLayout > maxWidth) {
                currentX = 0;
                currentXLayout = 0;
                currentY += font.getSize() + marginVertical;
            }
            currentX += wordWidth + marginHorizontal;
            currentXLayout += wordWidthLayout + marginHorizontal;
        }

        return currentY + font.getSize() + marginVertical;
    } else {
        return font.getSize() + marginVertical + 20.f;
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

    window = glfwCreateWindow(kWidth, kHeight, "C++ Lyrics Sample APP", NULL, NULL);
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
    double lastTimeFrame = glfwGetTime();

    int lastFocusedLine = -1;
    float lastLineHeight = 0.f;

    float estimatedHeightMap[300];

    while (!glfwWindowShouldClose(window)) {
        // Measure fps
        double currentTime = glfwGetTime();
        nbFrames++;

        if (currentTime - lastTime >= 1.0) {
            lastFPS = nbFrames;
            nbFrames = 0;
            lastTime = currentTime;
        }

        // get frame passed

        double currentTimeFrame = glfwGetTime();
        double deltaTime = (currentTimeFrame - lastTimeFrame) * 1000;
        lastTimeFrame = currentTimeFrame;
        double framePassed = deltaTime / (1000. / 60.);

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


        int focusedLineNum = lines.size() - 1;
        for (int i = 0; i < lines.size() - 1; i++) {
            const auto &line = lines[i];
            const auto &nextLine = lines[i + 1];
            if (line.start <= t && line.end >= t || nextLine.start >= t) {
                focusedLineNum = i;
                lyric_ctx.currentLine.animateTo(i);
                break;
            }
        }

        float focusY = 200.f;

        if (focusedLineNum != lastFocusedLine) {
            lastLineHeight = focusedLineNum == 0 ? 0.f : estimatedHeightMap[focusedLineNum - 1];
            lastFocusedLine = focusedLineNum;
        }
        float currentFocusedLineY = focusY + (focusedLineNum - lyric_ctx.currentLine.current) * lastLineHeight;
        float currentY = currentFocusedLineY;// pos of last focused line


        for (int x = focusedLineNum - 1; x >= 0; x--) {
            if (currentY < 0) break;
            const auto &line = lines[x];
            const float distToFocus = std::abs(lyric_ctx.currentLine.current - x);
            const float fontSize = std::clamp(30.f * (2 - distToFocus) / 2 + 30.f, 30.f, 60.f);
            float lineHeight = estimateLyricLineHeight(line, kWidth - 420.f,
                                                       SkFont(typeface, fontSize), SkFont(typeface, 60.f));

            currentY -= lineHeight;
            renderLyricLine(canvas, t, line, 400.f, currentY, kWidth - 420.f,
                            SkFont(typeface, fontSize), SkFont(typeface, 60.f), distToFocus * 0.04);
        }
        currentY = currentFocusedLineY;// pos of current focused line
        for (int x = focusedLineNum; x < lines.size(); x++) {
            const auto &line = lines[x];
            if (currentY > kHeight) {
                break;
            }
            const float distToFocus = std::abs(lyric_ctx.currentLine.current - x);
            float lineHeight = renderLyricLine(canvas, t, line, 400.f, currentY, kWidth - 420.f,
                                               SkFont(typeface, std::clamp(30.f * (2 - distToFocus) / 2 + 30.f, 30.f, 60.f)), SkFont(typeface, 60.f), distToFocus * 0.04);

            estimatedHeightMap[x] = lineHeight;
            currentY += lineHeight;
        }
        //        for (int x = 0; x < lines.size(); x++) {
        //            const auto &line = lines[x];
        //
        //            currentY += std::clamp(50.f - std::abs(currentY - focusY), 0.f, 50.f) + 10.f;
        //            const auto distToFocus = std::abs(currentY - focusY);
        //            const static float sizeChangeRange = 300.f;
        //
        //
        //            if (x == focusedLineNum) {
        //                std::cout << (currentY - focusY) << " " << lastFocusedLine << std::endl;
        //                if (lastFocusedLine != focusedLineNum) {
        //                    lastFocusedLine = focusedLineNum;
        //                }
        //
        //                lyric_ctx.offsetY.target = lyric_ctx.offsetY.current - currentY + focusY;
        //            }
        //
        //            if (currentY < -200 && estimatedHeightMap[x]) {
        //                currentY += estimatedHeightMap[x];
        //                continue;
        //            }
        //            float lineHeightEstimated =
        //                    estimateLyricLineHeight(line, kWidth - 20.f,
        //                                            SkFont(typeface, std::clamp(30.f * (1 - distToFocus / sizeChangeRange) + 30.f, 30.f, 60.f)),
        //                                            SkFont(typeface, 60.f));
        //            estimatedHeightMap[x] = lineHeightEstimated;
        //
        //
        //            if (lineHeightEstimated + currentY < 0) {
        //                currentY += lineHeightEstimated;
        //                continue;
        //            }
        //
        //            if (currentY > kHeight) break;
        //
        //                    float lineHeight = renderLyricLine(canvas, t, line, 400.f, currentY, kWidth - 20.f,
        //                                                       SkFont(typeface, std::clamp(30.f * (1 - distToFocus / sizeChangeRange) + 30.f, 30.f, 60.f)), SkFont(typeface, 60.f), distToFocus * 0.01);
        //                    currentY += lineHeight;
        //        }

        sContext->flush();

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            t += 140.f * framePassed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            t -= 140.f * framePassed;

        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
            t = 0.f;

        t += deltaTime;

        // animate ctx

#define DO_ANIMATE_FLOAT_EASE_IN_OUT(name)                                                  \
    lyric_ctx.name.animateState += lyric_ctx.name.stepPerFrame * framePassed;               \
    lyric_ctx.name.current = lyric_ctx.name.target *                                        \
                                     (1 - std::pow(2, -10 * lyric_ctx.name.animateState)) + \
                             lyric_ctx.name.current * std::pow(2, -10 * lyric_ctx.name.animateState);

#define DO_ANIMATE_FLOAT_EASE_IN_VELOCITY(name) \
    lyric_ctx.name.current += (lyric_ctx.name.target - lyric_ctx.name.current) * 0.07f * framePassed;

#define DO_ANIMATE_FLOAT_HALF(name) \
    lyric_ctx.name.current = (lyric_ctx.name.target + lyric_ctx.name.current) / 2;

        DO_ANIMATE_FLOAT_HALF(offsetY);
        DO_ANIMATE_FLOAT_EASE_IN_OUT(currentLine);

        glfwSwapBuffers(window);
    }

    cleanup_skia();

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}