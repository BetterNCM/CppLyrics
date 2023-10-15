#include "Lyric.h"
#include "LyricParser.h"
#include "pch.h"
#include "render/DynamicLyricWordRendererNormal.h"


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

static const float marginBottom = 20.f;

float renderLyricLine(
        SkCanvas *canvas,
        float relativeTime,
        const LyricLine &line,
        float x, float y,
        float maxWidth,
        const SkFont &font,
        const SkFont &layoutFont,
        float blur = 0.f) {
    float marginVertical = 10.f;
    float marginHorizontal = 0.f;


    float height = 0.f;
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

            renderer.renderLyricWord(canvas, relativeTime - line.start, word, currentX, currentY, font, blur);
            currentX += wordWidth + marginHorizontal;
            currentXLayout += wordWidthLayout + marginHorizontal;
        }

        height += currentY - y + font.getSize() + marginVertical + marginBottom;
    } else {
        SkPaint paint;
        canvas->drawString(std::get<1>(line.lyric).c_str(), x, y, font, paint);
    }

    return height;
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

        return currentY + font.getSize() + marginVertical + marginBottom;
    } else {
        return font.getSize() + marginVertical + marginBottom;
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

    // read from lyric.txt
    const auto lyricFile = std::ifstream("lyric.txt");
    std::stringstream buffer;
    buffer << lyricFile.rdbuf();
    const auto lyricStr = buffer.str();

    const auto lines = LyricParser::parse(lyricStr);

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
                        std::format("FPS: {} Time: {:.2f}", lastFPS, t).c_str(), SkFont(typeface, 15)),
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

        float focusY = 150.f;

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
                            SkFont(typeface, fontSize), SkFont(typeface, 60.f), distToFocus * 0.8);
        }
        currentY = currentFocusedLineY;// pos of current focused line
        for (int x = focusedLineNum; x < lines.size(); x++) {
            const auto &line = lines[x];
            if (currentY > kHeight) {
                break;
            }
            const float distToFocus = std::abs(lyric_ctx.currentLine.current - x);
            float lineHeight = renderLyricLine(canvas, t, line, 400.f, currentY, kWidth - 420.f,
                                               SkFont(typeface, std::clamp(30.f * (2 - distToFocus) / 2 + 30.f, 30.f, 60.f)), SkFont(typeface, 60.f), distToFocus * 0.8);

            estimatedHeightMap[x] = lineHeight;
            currentY += lineHeight;
        }

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