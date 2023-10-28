#include "Lyric.h"
#include "LyricParser.h"
#include "pch.h"
#include "render/DynamicLyricWordRendererNormal.h"
#include "render/RenderTextWrap.h"

#define NOMINMAX
#include "windows.h"

static const bool enableLowEffect = false;

GrDirectContext *sContext = nullptr;
SkSurface *sSurface = nullptr;
GrBackendRenderTarget backendRenderTarget;
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

    if (!sContext) {
        auto interface1 = GrGLMakeNativeInterface();
        sContext = GrDirectContext::MakeGL(interface1).release();
    }

    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = 0;// assume default framebuffer
    // We are always using OpenGL, and we use RGBA8 internal format for both RGBA and BGRA configs in OpenGL.
    //(replace line below with this one to enable correct color spaces) framebufferInfo.fFormat = GL_SRGB8_ALPHA8;
    framebufferInfo.fFormat = GL_RGBA8;

    SkColorType colorType = kRGBA_8888_SkColorType;
    backendRenderTarget = GrBackendRenderTarget(w, h,
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

int kWidth = 630;
int kHeight = 900;


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
    ANIMATED_FLOAT(currentLine, 0.f, 0.03f)
} lyric_ctx;

static const float marginBottom = 0.f;

float renderLyricLine(
        SkCanvas *canvas,
        float relativeTime,
        const LyricLine &line,
        float x, float y,
        float maxWidth,
        const SkFont &font,
        const SkFont &layoutFont,
        const SkFont &fontSubLyrics,
        float blur = 0.f) {
    float marginVertical = 10.f;
    float marginHorizontal = 0.f;


    static const auto renderer = DynamicLyricWordRendererNormal();

    float currentX = x;
    float currentXLayout = x;
    float currentY = y;

    if (line.isDynamic) {
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

    } else {
        SkPaint paint;
        canvas->drawString(std::get<1>(line.lyric).c_str(), x, y, font, paint);
    }

    currentY += font.getSize() + marginVertical * 2;

    for (const auto &subLyric: line.subLyrics) {
        auto paint = SkPaint();
        paint.setColor(SkColorSetARGB(0x70, 0xFF, 0xFF, 0xFF));
        paint.setBlendMode(SkBlendMode::kPlus);
        if (blur > 0.f) {
            paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blur, false));
        }
        currentY += renderTextWithWrap(*canvas, paint, fontSubLyrics, fontSubLyrics, maxWidth, x, currentY, subLyric) + 8.f;
    }

    return currentY - y + marginBottom;
}

float estimateLyricLineHeight(
        const LyricLine &line,
        float maxWidth,
        const SkFont &font,
        const SkFont &layoutFont,
        const SkFont &fontSubLyrics) {
    static const float marginVertical = 10.f;
    static const float marginHorizontal = 10.f;
    static const auto renderer = DynamicLyricWordRendererNormal();

    float currentX = 0;
    float currentXLayout = 0;
    float currentY = 0;

    if (line.isDynamic) {
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

    } else {
        return font.getSize() + marginVertical + marginBottom;
    }

    currentY += font.getSize() + marginVertical;

    for (const auto &subLyric: line.subLyrics)
        currentY += measureTextWithWrap(fontSubLyrics, fontSubLyrics, maxWidth, subLyric) + 8.f;


    return currentY + marginBottom;
}

// TODO: thread safety is not guaranteed, refactor later
std::shared_ptr<std::vector<LyricLine>> _lines_ref = std::make_shared<std::vector<LyricLine>>();
std::atomic<float> currentTimeExt = -1.f;
std::atomic<bool> isPaused = false;
std::atomic<std::shared_ptr<std::string>> songName = std::make_shared<std::string>();
std::atomic<std::shared_ptr<std::string>> songArtist = std::make_shared<std::string>();
std::atomic<std::array<float, 3> *> songColor1 = nullptr;
std::atomic<std::array<float, 3> *> songColor2 = nullptr;

sk_sp<SkImage> songCover = nullptr;

float t = 0.f;
float fluidTime = 0.f;

void renderScrollingString(SkCanvas &canvas, SkFont &font, SkPaint &paint, int maxWidth, float t, int x, int y, const char *text) {
    const auto textWidth = font.measureText(text, strlen(text), SkTextEncoding::kUTF8);
    if (textWidth > maxWidth) {
        // double render, scroll
        const float fontSize = font.getSize();
        const auto scrollCycle = 40000.f;//ms
        const auto padding = 0.5f * fontSize;
        const auto scrollWidth = textWidth + padding * 2.f;

        const auto currentProgress = t / scrollCycle;
        /* 70% time stop, 30% time scrolling */
        const auto scrollProgress = std::clamp((currentProgress - 0.6f) / 0.4f, 0.f, 1.f);
        const auto scrollX = -scrollWidth * scrollProgress;

        font.setSubpixel(true);
        canvas.drawString(text, x + scrollX, y, font, paint);
        canvas.drawString(text, x + scrollX + scrollWidth, y, font, paint);
    } else {
        canvas.drawString(text, x, y, font, paint);
    }
}

void renderSongInfo(SkCanvas &canvas, SkFont &font, SkFont &fontMinorInfo, bool smallMode, int maxWidth) {
    if (songCover == nullptr) return;
    const auto pic = songCover;
    float dx = kWidth / 6 - 100, dy = std::max(kHeight / 2 - 300, 100), dw = std::max(std::clamp(kWidth / 4.f, 200.f, 400.f), std::clamp(kHeight / 4.f, 200.f, 400.f)),
          dh = dw;
    if (smallMode) {
        dx = 20, dy = 40, dw = 100, dh = 100;
    }

    // mask path
    SkPath path;
    path.addRoundRect(
            SkRect::MakeXYWH(dx, dy, dw, dh),
            20.f, 20.f);

    // draw background glow
    SkPaint paint;
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 20.f, true));
    paint.setColor(SkColorSetARGB(0x60, (int) (*songColor1).at(0), (int) (*songColor1).at(1), (int) (*songColor1).at(2)));
    canvas.drawPath(path, paint);

    // draw image
    canvas.save();
    canvas.clipPath(path, true);
    const auto matrix =
            SkMatrix::MakeRectToRect(
                    SkRect::MakeWH(pic->width(), pic->height()),
                    SkRect::MakeWH(dw, dh).makeOffset(dx, dy),
                    SkMatrix::kCenter_ScaleToFit);

    canvas.setMatrix(matrix);
    canvas.drawImage(pic, 0, 0);
    canvas.restore();
    // draw song info

    float songInfoX = dx;
    float songInfoY = dy + dh + 90.f;

    if (smallMode) {
        songInfoX = dx + dw + 20.f;
        songInfoY = dy + dh / 2.f + 5.f;
    }

    float songInfoWidth = maxWidth - songInfoX;
    if (smallMode) {
        songInfoWidth = kWidth - songInfoX;
    }

    constexpr float songInfoPadding = 10.f;
    paint = SkPaint();

    SkPoint pts[2] = {{songInfoX + songInfoWidth * -0.05f, 0},
                      {songInfoX + songInfoWidth * 1.05f, 0}};
    SkColor colors[4] = {SkColorSetARGB(0, 255, 255, 255),
                         SkColorSetARGB(170, 255, 255, 255),
                         SkColorSetARGB(170, 255, 255, 255),
                         SkColorSetARGB(0, 255, 255, 255)};
    SkScalar pos[4] = {0.f, 0.05f / 1.1f, 1.05f / 1.1f, 1.f};
    const auto shader = SkGradientShader::MakeLinear(pts,
                                                     colors,
                                                     pos, 4, SkTileMode::kClamp);

    paint.setAntiAlias(true);
    paint.setShader(shader);
    paint.setBlendMode(SkBlendMode::kPlus);
    renderScrollingString(canvas, font, paint, songInfoWidth, fluidTime, songInfoX, songInfoY, songName.load()->c_str());

    songInfoY += font.getSize() + (smallMode ? -20.f : -20.f);
    paint.setColor(SkColorSetARGB(80, 255, 255, 255));
    renderScrollingString(canvas, fontMinorInfo, paint, songInfoWidth, fluidTime, songInfoX, songInfoY, songArtist.load()->c_str());
}


int initCppLyrics() {
    GLFWwindow *window;
    glfwSetErrorCallback(error_callback);
    bool enableFrameLimit = false;
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    //   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //  glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    // disable double buffer
    glfwWindowHint(GLFW_DOUBLEBUFFER, enableFrameLimit);
    glfwWindowHint(GLFW_DECORATED, 0);

    glfwWindowHint(GLFW_RESIZABLE, 1);
    //    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);

    window = glfwCreateWindow(kWidth, kHeight, "C++ Lyrics", NULL, NULL);
//    while (!glfwWindowShouldClose(window)) {
//        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//        glClear(GL_COLOR_BUFFER_BIT);
//        glBegin(GL_QUADS);
//        glColor3f(0, 0, 1);
//        glVertex3f(-0.5, -0.5, -1);
//        glColor3f(0, 1, 0);
//        glVertex3f(0.5, -0.5, -1);
//        glColor3f(1, 0, 1);
//        glVertex3f(0.5, 0.5, -1);
//        glColor3f(1, 1, 0);
//        glVertex3f(-0.5, 0.5, -1);
//        glEnd();
//
//        // swap front and back buffers
//        glfwSwapBuffers(window);
//
//        // poll for and process events
//        glfwPollEvents();
//    }
#ifdef _WIN32
    void processWindow(GLFWwindow * hwnd);
    processWindow(window);
#endif


    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    if (!enableFrameLimit)
        glfwSwapInterval(0);

    init_skia(kWidth, kHeight);

    glfwSetKeyCallback(window, key_callback);

    // Draw to the surface via its SkCanvas.
    SkCanvas *canvas = sSurface->getCanvas(); // We don't manage this pointer's lifetime.
    glfwSetWindowUserPointer(window, &canvas);// TODO: refactor to class, this should be ptr to class

    glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        kWidth = width;
        kHeight = height;
        init_skia(width, height);
        *((SkCanvas **) glfwGetWindowUserPointer(window)) = sSurface->getCanvas();
    });

    // calc framerate
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    int lastFPS = 0;

    float minFPS = 100000.f, maxFPS = 0.f;
    sk_sp<SkFontMgr> fontMgr = SkFontMgr::RefDefault();

    auto fontFamily = fontMgr->matchFamily("Noto Sans SC");
    // fallback to Microsoft YaHei
    if (!fontFamily->count()) {
        fontFamily = fontMgr->matchFamily("Microsoft YaHei");
    }
    const auto typefaceBold = sk_sp(fontFamily->matchStyle(
            SkFontStyle::Bold()));
    const auto typefaceMed = sk_sp(fontFamily->matchStyle(
            SkFontStyle::Normal()));

    // load ./bg.png
    //    auto data = SkData::MakeFromFileName("bg.png");
    //    auto pic = SkImage::MakeFromEncoded(data);
    double lastTimeFrame = glfwGetTime();

    int lastFocusedLine = -1;
    float lastLineHeight = 0.f;

    float estimatedHeightMap[2000];

    const auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(R"(
uniform float iTime;
uniform vec2 iResolution;

uniform vec4 fluidColor1;
uniform vec4 fluidColor2;

//	Classic Perlin 3D Noise
//	by Stefan Gustavson
//
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}
vec3 fade(vec3 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}

float cnoise(vec3 P){
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod(Pi0, 289.0);
  Pi1 = mod(Pi1, 289.0);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 / 7.0;
  vec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 / 7.0;
  vec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
  return 2.2 * n_xyz;
}
half4 main(vec2 fragCoord)
{
    vec2 uv = 2.4 * fragCoord.xy / vec2(max(iResolution.x, iResolution.y));

    float p = cnoise(vec3(iTime, uv.x, uv.y));

//    half4 col = mix(half4(1.,.5,.1,1.), half4(0.1,.5,1.,1.), p*.02+.4);

    return half4(mix(fluidColor1/256, fluidColor2 / 256, p));
}
)"));

    if (!effect) {
        printf("SkRuntimeEffect error: %s\n", err.c_str());
        exit(1);
    }


    while (!glfwWindowShouldClose(window)) {

        const auto lines = *_lines_ref;

        if (lines.size() == 0) {
            glfwPollEvents();
            continue;
        }
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        //        glClearColor(54 / 255.f, 52 / 255.f, 57 / 255.f, 255 / 255.f);
        glClear(GL_COLOR_BUFFER_BIT);
        // Measure fps
        double currentTime = glfwGetTime();
        nbFrames++;

        if (currentTime - lastTime >= .3) {
            lastFPS = nbFrames / (currentTime - lastTime);
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
        // make iTime uniform
        SkRuntimeShaderBuilder builder(effect);
        builder.uniform("iTime") = fluidTime / 10000;
        builder.uniform("iResolution") = SkV2{(float) kWidth, (float) kHeight};
        if (songColor1) {
            const auto sc1 = *songColor1;
            const auto sc2 = *songColor2;
            builder.uniform("fluidColor1") = SkV4{sc1.at(0), sc1.at(1), sc1.at(2), 256};
            builder.uniform("fluidColor2") = SkV4{sc2.at(0), sc2.at(1), sc2.at(2), 256};
        } else {
            builder.uniform("fluidColor1") = SkV4{54, 52, 57, 256};
            builder.uniform("fluidColor2") = SkV4{255, 52, 0, 256};
        }


        //        paint.setShader(builder.makeShader());
        // use half opacity paint fluidColor1
        paint.setColor(SkColorSetARGB(0x80, (int) (*songColor1).at(0), (int) (*songColor1).at(1), (int) (*songColor1).at(2)));
        paint.setBlendMode(SkBlendMode::kSrc);

        canvas->drawPaint(paint);
        //        canvas->drawImage(
        //                pic, 0, 0);
        paint = SkPaint();
        paint.setColor(SkColorSetARGB(60, 255, 255, 255));
        canvas->drawTextBlob(
                SkTextBlob::MakeFromString(
                        std::format("FPS: {} Time: {:.2f}", lastFPS, t).c_str(), SkFont(typefaceBold, 15)),
                10, 30, paint);


        int focusedLineNum = lines.size();
        for (int i = 0; i < lines.size() - 1; i++) {
            const auto &line = lines[i];
            const auto &nextLine = lines[i + 1];
            if (line.start <= t && line.end >= t || nextLine.start >= t) {
                focusedLineNum = i;
                lyric_ctx.currentLine.animateTo(i);
                break;
            }
        }

        float focusY = std::max(kHeight / 2 - 300, 100);
        float X;
        bool smallMode = false;
        if (kWidth > 1000) {
            X = kWidth / 2;
        } else {
            X = 20.f;
            focusY = 180.f;
            smallMode = true;
        }

        if (focusedLineNum != lastFocusedLine) {
            lastLineHeight = focusedLineNum == 0 ? 0.f : estimatedHeightMap[focusedLineNum - 1];
            lastFocusedLine = focusedLineNum;
        }

        static auto boldFont = SkFont(typefaceBold, 60.f);
        static auto midFont = SkFont(typefaceMed, 30.f);

        renderSongInfo(*canvas, boldFont, midFont, smallMode,
                       smallMode ? kWidth - 20.f : kWidth / 2 - 100.f);

        float currentFocusedLineY = focusY + (focusedLineNum - lyric_ctx.currentLine.current) * lastLineHeight;
        float currentY = currentFocusedLineY;// pos of last focused line


        const static float minTextSize = 40.f, maxTextSize = 60.f;
        static auto subLyricFont = SkFont(typefaceBold, 30.f);
        static auto layoutFont = SkFont(typefaceBold, maxTextSize);
        if (!smallMode)
            for (int x = focusedLineNum - 1; x >= 0; x--) {
                if (currentY < 0) break;
                const auto &line = lines[x];
                const float distToFocus = std::abs(lyric_ctx.currentLine.current - x);
                const float fontSize = std::clamp((maxTextSize - minTextSize) * (2 - distToFocus) / 2 + minTextSize, minTextSize, maxTextSize);
                float lineHeight = estimateLyricLineHeight(line, kWidth - X - 10.f,
                                                           SkFont(typefaceBold, fontSize),
                                                           layoutFont,
                                                           subLyricFont);

                currentY -= lineHeight;
                renderLyricLine(canvas, t, line, X, currentY, kWidth - X - 10.f,
                                SkFont(typefaceBold, fontSize), layoutFont,
                                subLyricFont, 0);
            }

        currentY = currentFocusedLineY;// pos of current focused line
        for (int x = focusedLineNum; x < lines.size(); x++) {
            const auto &line = lines[x];
            if (currentY > kHeight) {
                break;
            }
            const float distToFocus = std::abs(lyric_ctx.currentLine.current - x);
            const float fontSize = std::clamp((maxTextSize - minTextSize) * (2 - distToFocus) / 2 + minTextSize, minTextSize, maxTextSize);

            float lineHeight = renderLyricLine(canvas, t, line, X, currentY, kWidth - X - 10.f,
                                               SkFont(typefaceBold, fontSize), layoutFont,
                                               subLyricFont, std::max(distToFocus * 0.8f - 4, 0.f));

            estimatedHeightMap[x] = lineHeight;
            currentY += lineHeight;
        }

        sContext->flush();

        if (currentTimeExt > 0) {
            t = currentTimeExt;
            currentTimeExt = -1.f;
        } else {
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                t += 140.f * framePassed;
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
                t -= 140.f * framePassed;

            if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
                t = 0.f;

            if (!isPaused)
                t += deltaTime;
        }

        fluidTime += deltaTime;


        // animate ctx

#define DO_ANIMATE_FLOAT_EASE_IN_OUT(name)                                                                \
    lyric_ctx.name.animateState += lyric_ctx.name.stepPerFrame * framePassed;                             \
    if (lyric_ctx.name.animateState < 1.0f)                                                               \
        lyric_ctx.name.current = lyric_ctx.name.target *                                                  \
                                         (1 - std::pow(2, -10 * lyric_ctx.name.animateState)) +           \
                                 lyric_ctx.name.current * std::pow(2, -10 * lyric_ctx.name.animateState); \
    else {                                                                                                \
        lyric_ctx.name.current = lyric_ctx.name.target;                                                   \
    }
#define DO_ANIMATE_FLOAT_EASE_IN_VELOCITY(name) \
    lyric_ctx.name.current += (lyric_ctx.name.target - lyric_ctx.name.current) * 0.07f * framePassed;

#define DO_ANIMATE_FLOAT_HALF(name) \
    lyric_ctx.name.current = (lyric_ctx.name.target + lyric_ctx.name.current) / 2;

        DO_ANIMATE_FLOAT_EASE_IN_OUT(currentLine);


        if (enableFrameLimit) glfwSwapBuffers(window);
        else
            glFlush();
    }

    cleanup_skia();

    glfwDestroyWindow(window);
    //    glfwTerminate();
    //    exit(EXIT_SUCCESS);
}