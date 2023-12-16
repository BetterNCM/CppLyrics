#include "main.h"
#include <cassert>

#define NOMINMAX
#include "windows.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "render/DynamicLyricWordRendererNormal.h"
#include "render/RenderTextWrap.h"

std::shared_ptr<std::vector<LyricLine>> _lines_ref = std::make_shared<std::vector<LyricLine>>();
std::atomic<float> currentTimeExt = -1.f;
std::atomic<bool> isPaused = false;
std::atomic<std::shared_ptr<std::string>> songName = std::make_shared<std::string>();
std::atomic<std::shared_ptr<std::string>> songArtist = std::make_shared<std::string>();
std::atomic<std::array<float, 3> *> songColor1 = nullptr;
std::atomic<std::array<float, 3> *> songColor2 = nullptr;
sk_sp<SkImage> songCover = nullptr;


CppLyrics::~CppLyrics() {
}


struct LyricWordRenderPos {
    float x;
    float y;
};

float CppLyrics::renderLyricLine(
        SkCanvas *canvas, float relativeTime, const LyricLine &line,
        float x, float y, float maxWidth, const SkFont &font,
        const SkFont &layoutFont, const SkFont &fontSubLyrics, float blur, bool overflowScroll) {


    static const auto renderer = DynamicLyricWordRendererNormal();
    float relativeLineTime = relativeTime - line.start;
    float currentX = x;
    float currentXLayout = x;
    float currentY = y;


    if (line.isDynamic) {
        std::vector<LyricWordRenderPos> wordWidths;
        float renderXOffset = 0.f;
        wordWidths.reserve(std::get<0>(line.lyric).size());

        const unsigned int size = std::get<0>(line.lyric).size();
        for (int i = 0; i < size; i++) {
            const auto &word = std::get<0>(line.lyric)[i];

            float wordWidth = renderer.measureLyricWord(word, font);
            float wordWidthLayout = renderer.measureLyricWord(word, layoutFont);
            if (currentXLayout + wordWidthLayout - x > maxWidth) {
                if (!overflowScroll) {
                    currentX = x;
                    currentXLayout = x;
                    currentY += font.getSize() + wordMargin;
                }
            }

            if (relativeLineTime > word.start && (relativeLineTime < word.end || i == size - 1)) {
                const auto progressX = std::min((relativeLineTime - word.start) / (word.end - word.start), 1.0f) * (wordWidth + wordMargin);
                const auto progressXAbs = progressX + currentX - x;
                const auto middleX = (maxWidth - 10.f) / 2.f;

                renderXOffset = std::max(0.f, progressXAbs - middleX);
            }


            wordWidths.emplace_back(currentX, currentY);
            currentX += wordWidth + wordMargin;
            currentXLayout += wordWidthLayout + wordMargin;
        }

        renderXOffset = std::clamp(renderXOffset, 0.f, std::max(currentX - x - maxWidth, 0.f));

        for (int i = 0; i < size; i++) {
            const auto &word = std::get<0>(line.lyric)[i];
            const auto &wordWidth = wordWidths[i];

            constexpr float disappearThreshold = 50.f;
            const float rate = 1 - std::clamp((wordWidth.x - renderXOffset - x + disappearThreshold) / disappearThreshold, 0.f, 1.f);
            renderer.renderLyricWord(canvas, relativeTime - line.start,
                                     word, wordWidth.x - renderXOffset, wordWidth.y, font, blur, showSongInfo ? 1 - sqrt(rate) : 1);
        }

    } else {
        SkPaint paint;
        canvas->drawString(std::get<1>(line.lyric).c_str(), x, y, font, paint);
    }


    currentY += font.getSize() + subLyricsMarginTop;

    const auto progress = std::clamp((relativeTime - line.start) / (line.end - line.start), 0.f, 1.f);

    for (const auto &subLyric: line.subLyrics) {
        auto paint = SkPaint();
        paint.setColor(SkColorSetARGB(0x70, 0xFF, 0xFF, 0xFF));
        paint.setBlendMode(SkBlendMode::kPlus);
        if (blur > 0.f) {
            paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blur, false));
        }
        if (useSingleLine) {
            const auto subLyricWidth = fontSubLyrics.measureText(subLyric.c_str(), subLyric.size(), SkTextEncoding::kUTF8);
            const auto offsetX = std::clamp((subLyricWidth - maxWidth) * progress, 0.f, std::max(subLyricWidth - maxWidth, 0.f));
            canvas->drawString(subLyric.c_str(), x - offsetX, currentY + fontSubLyrics.getSize(), fontSubLyrics, paint);
            currentY += fontSubLyrics.getSize() + 8.f;
        } else {
            currentY += renderTextWithWrap(*canvas, paint, fontSubLyrics, fontSubLyrics, maxWidth, x, currentY, subLyric) + 8.f;
        }
    }

    return currentY - y + marginBottomLyrics;
}
float CppLyrics::estimateLyricLineHeight(
        const LyricLine &line, float maxWidth, const SkFont &font,
        const SkFont &layoutFont, const SkFont &fontSubLyrics, bool overflowScroll) {
    static const auto renderer = DynamicLyricWordRendererNormal();

    float currentX = 0;
    float currentXLayout = 0;
    float currentY = 0;

    if (line.isDynamic) {
        for (const auto &word: std::get<0>(line.lyric)) {
            float wordWidth = renderer.measureLyricWord(word, font);
            float wordWidthLayout = renderer.measureLyricWord(word, layoutFont);
            if (currentXLayout + wordWidthLayout > maxWidth && !overflowScroll) {
                currentX = 0;
                currentXLayout = 0;
                currentY += font.getSize() + wordMargin;
            }
            currentX += wordWidth + wordMargin;
            currentXLayout += wordWidthLayout + wordMargin;
        }

    } else {
        return font.getSize() + wordMargin + marginBottomLyrics;
    }

    currentY += font.getSize() + subLyricsMarginTop;

    for (const auto &subLyric: line.subLyrics)
        currentY += measureTextWithWrap(fontSubLyrics, fontSubLyrics, maxWidth, subLyric) + 8.f;


    return currentY + marginBottomLyrics;
}
void CppLyrics::renderScrollingString(SkCanvas &canvas, SkFont &font, SkPaint &paint, int maxWidth, float t, int x, int y, const char *text) {
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
void CppLyrics::renderSongInfo(SkCanvas &canvas, SkFont &font, SkFont &fontMinorInfo, bool smallMode, int maxWidth) {
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

CppLyrics::CppLyrics() {
}
void CppLyrics::render(SkCanvas *canvas) {
    //    double lastTime = glfwGetTime();
    //    int nbFrames = 0;
    //    int lastFPS = 0;

    //    float minFPS = 100000.f, maxFPS = 0.f;
    static sk_sp<SkFontMgr> fontMgr = SkFontMgr::RefDefault();
    static auto fontFamily = fontMgr->matchFamily("Noto Sans SC");
    // fallback to Microsoft YaHei
    if (!fontFamily->count()) {
        fontFamily = fontMgr->matchFamily("Microsoft YaHei");
    }
    static const auto typefaceBold = sk_sp(fontFamily->matchStyle(
            SkFontStyle::Bold()));
    static const auto typefaceMed = sk_sp(fontFamily->matchStyle(
            SkFontStyle::Normal()));

    // load ./bg.png
    //    auto data = SkData::MakeFromFileName("bg.png");
    //    auto pic = SkImage::MakeFromEncoded(data);
    //    double lastTimeFrame = glfwGetTime();

    static int lastFocusedLine = -1;
    static float lastLineHeight = 0.f;

    static float estimatedHeightMap[2000];

    static const auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(R"(
uniform float iTime;
uniform vec2 iResolution;
uniform vec2 iImageResolution;
uniform vec2 color1;
uniform vec2 color2;
uniform shader iImage1;
uniform vec4 fluidColor1;
uniform vec4 fluidColor2;
uniform float opacity;

half4 shaderBlend(vec2 fragCoord) {
    return mix(fluidColor1 / 256, fluidColor2 / 256, fragCoord.x / iResolution.x);
}

const float r = 10;
const float pi = 3.1415926;
const float ste = 0.5;
half4 blurred(float2 fragCoord)
{
	vec2 uv = fragCoord.xy / iImageResolution.xy;
  	half4 t = iImage1.eval(fragCoord.xy);
	float q=0;
  	for(float i=-pi;i<pi;i+=ste) {
    	t+=iImage1.eval(vec2(
           fragCoord.x + sin(i) * r,
           fragCoord.y + cos(i) * r
        ));
        q++;
    }
  	return t / q;
}


half4 main(vec2 fragCoord) {
    float iTime = iTime / 8;
    vec2 uv = fragCoord.xy / iImageResolution.xy;
    vec2 p=(fragCoord.xy / 4 - iImageResolution.xy)/max(iImageResolution.x,iImageResolution.y);
    float2 scale = iImageResolution.xy / iResolution.xy;

    for(int i=1;i<45;i++)
    {
        vec2 newp=p;
        newp.x+=(0.5/float(i))*cos(float(i)*p.y+iTime*12.0/8.0+0.03*float(i))+1.3;
        newp.y+=(0.5/float(i))*cos(float(i)*p.x+iTime*22.0/8.0+0.03*float(i+10))+1.2;
        p=newp;
    }

  	half4 v1 = blurred(mod(p * (scale / 1.5 + iTime / 100 + 35), iImageResolution.xy));
  	half4 v2 = shaderBlend(p);
  	return vec4(mix(v1, v2, clamp(length(v1) - length(v2*1.6), 0.1, 0.8)).xyz / 1.3, opacity);
}
)"));

    if (!effect) {
        printf("SkRuntimeEffect error: %s\n", err.c_str());
        exit(1);
    }


    const auto lines = *_lines_ref;

    if (lines.size() == 0) {
        return;
    }

    double currentTime = glfwGetTime();

    SkPaint paint;
    if (useFluentBg) {
        // make iTime uniform
        SkRuntimeShaderBuilder builder(effect);
        builder.uniform("iTime") = fluidTime / 10000;
        builder.uniform("iResolution") = SkV2{(float) kWidth, (float) kHeight};

        builder.child("iImage1") = songCover->makeRawShader(SkSamplingOptions{});
        builder.uniform("opacity") = opacity;
        builder.uniform("iImageResolution") = SkV2{(float) songCover->width(), (float) songCover->height()};
        if (songColor1) {
            const auto sc1 = *songColor1;
            const auto sc2 = *songColor2;
            builder.uniform("fluidColor1") = SkV4{sc1.at(0), sc1.at(1), sc1.at(2), 256};
            builder.uniform("fluidColor2") = SkV4{sc2.at(0), sc2.at(1), sc2.at(2), 256};
        } else {
            builder.uniform("fluidColor1") = SkV4{54, 52, 57, 256};
            builder.uniform("fluidColor2") = SkV4{255, 52, 0, 256};
        }
        paint.setBlendMode(SkBlendMode::kSrc);
        paint.setShader(builder.makeShader());
    } else {// use half opacity paint fluidColor1
        paint.setBlendMode(SkBlendMode::kSrc);
        paint.setColor(SK_ColorTRANSPARENT);
    }

    canvas->drawPaint(paint);
    //        canvas->drawImage(
    //                pic, 0, 0);
    paint = SkPaint();
    paint.setColor(SkColorSetARGB(40, 255, 255, 255));
    if (showTips)
        canvas->drawTextBlob(
                SkTextBlob::MakeFromString(
                        std::format("Bg(g): {} Blur(b): {} Resize(r): {} SingleLine(s): {} | FPS: {} Time: {:.2f} ", useFluentBg, useFontBlur, useTextResize, useSingleLine, lastFPS, t).c_str(), SkFont(typefaceBold, 15)),
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
    if (showSongInfo) {
        if (kWidth > 1000) {
            X = kWidth / 2;
        } else {
            X = 20.f;
            focusY = 180.f;
            smallMode = true;
        }
    } else {
        X = 20.f;
        focusY = 3.f;
    }


    if (focusedLineNum != lastFocusedLine) {
        lastLineHeight = focusedLineNum == 0 ? 0.f : estimatedHeightMap[focusedLineNum - 1];
        lastFocusedLine = focusedLineNum;
    }

    static auto boldFont = SkFont(typefaceBold, 60.f);
    static auto midFont = SkFont(typefaceMed, 30.f);

    if (showSongInfo)
        renderSongInfo(*canvas, boldFont, midFont, smallMode,
                       smallMode ? kWidth - 20.f : kWidth / 2 - 100.f);

    float currentFocusedLineY = focusY + (focusedLineNum - lyric_ctx.currentLine.current) * lastLineHeight;
    float currentY = currentFocusedLineY;// pos of last focused line

    auto subLyricFont = SkFont(typefaceBold, minTextSize);
    auto layoutFont = SkFont(typefaceBold, maxTextSize);
    if (!smallMode || !showSongInfo)
        for (int x = focusedLineNum - 1; x >= 0; x--) {
            if (currentY < 0) break;
            const auto &line = lines[x];
            const float distToFocus = std::abs(lyric_ctx.currentLine.current - x);
            const float fontSize = useTextResize ? std::clamp((maxTextSize - minTextSize) * (2 - distToFocus) / 2 + minTextSize, minTextSize, maxTextSize) : maxTextSize;
            const float lineHeight = estimateLyricLineHeight(line, kWidth - X - 10.f,
                                                             SkFont(typefaceBold, fontSize),
                                                             layoutFont,
                                                             subLyricFont, useSingleLine);

            currentY -= lineHeight;
            const float lineHeightReal = renderLyricLine(canvas, t, line, X, currentY, kWidth - X - 10.f,
                                                         SkFont(typefaceBold, fontSize), layoutFont,
                                                         subLyricFont, 0, useSingleLine);

            assert(std::abs(lineHeight - lineHeightReal) < 1.f);
        }

    currentY = currentFocusedLineY;// pos of current focused line
    for (int x = focusedLineNum; x < lines.size(); x++) {
        const auto &line = lines[x];
        if (currentY > kHeight) {
            break;
        }
        const float distToFocus = std::abs(lyric_ctx.currentLine.current - x);
        const float fontSize = useTextResize ? std::clamp((maxTextSize - minTextSize) * (2 - distToFocus) / 2 + minTextSize, minTextSize, maxTextSize) : maxTextSize;
        auto curFont = SkFont(typefaceBold, fontSize);
        curFont.setSubpixel(true);
        curFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);

        float lineHeight = renderLyricLine(canvas, t, line, X, currentY, kWidth - X - 10.f,
                                           curFont, layoutFont,
                                           subLyricFont, useFontBlur ? std::max(distToFocus * 0.8f - 4, 0.f) : 0.f, useSingleLine);

        estimatedHeightMap[x] = lineHeight;
        currentY += lineHeight;
    }
}


void CppLyrics::animate(const double deltaTime) {
    if (currentTimeExt > 0) {
        t = currentTimeExt;
        currentTimeExt = -1.f;
    } else {
        if (!isPaused.load())
            t += deltaTime;
    }

    double framePassed = deltaTime / (1000. / 60.);

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
}
