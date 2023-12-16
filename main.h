#pragma once
#include "Lyric.h"
#include "data/DataSource.h"
#include "data/LyricParser.h"
#include "pch.h"


static const bool enableLowEffect = false;


#define drawHorizontalLineAt(y, color)               \
    {                                                \
        SkPaint ___paint;                            \
        ___paint.setColor(SkColors::color);          \
        ___paint.setStrokeWidth(1.f);                \
        canvas->drawLine(0, y, kWidth, y, ___paint); \
    }

class CppLyrics {
    int lastFocusedLine = -1;
    float lastLineHeight = 0.f;
    float estimatedHeightMap[2000];

    sk_sp<SkFontMgr> fontMgr = SkFontMgr::RefDefault();
    sk_sp<SkFontStyleSet> fontFamily = sk_sp(fontMgr->matchFamily("Noto Sans SC"));
    const sk_sp<SkTypeface> typefaceBold = sk_sp(fontFamily->matchStyle(
            SkFontStyle::Bold()));
    const sk_sp<SkTypeface> typefaceMed = sk_sp(fontFamily->matchStyle(
            SkFontStyle::Normal()));

public:
    ~CppLyrics();
    CppLyrics(const CppLyrics &cppLyrics) = delete;

    DataSource *dataSource;
    int kWidth = 630;
    int kHeight = 900;
    int lastFPS = 0;
    float minTextSize = 25.f, maxTextSize = 50.f, wordMargin = 10.f;
    bool showSongInfo = true;
    bool showTips = true;
    float subLyricsMarginTop = 20.f, marginBottomLyrics = 10.f;

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
        ANIMATED_FLOAT(currentLine, 0.f, 0.18f)
    } lyric_ctx;

    constexpr static const float marginBottom = 0.f;

    float renderLyricLine(
            SkCanvas *canvas, float relativeTime, const LyricLine &line,
            float x, float y, float maxWidth, const SkFont &font,
            const SkFont &layoutFont, const SkFont &fontSubLyrics,
            float blur = 0.f, bool overflowScroll = false);

    float estimateLyricLineHeight(
            const LyricLine &line,
            float maxWidth,
            const SkFont &font,
            const SkFont &layoutFont,
            const SkFont &fontSubLyrics,
            bool overflowScroll = false);

    float t = 0.f;
    float fluidTime = 0.f;
    bool useFluentBg = true;
    bool useFontBlur = false;
    bool useTextResize = false;
    bool useSingleLine = false;
    bool useRoundBorder = false;
    float opacity = 1;
    void renderScrollingString(SkCanvas &canvas, SkFont &font, SkPaint &paint, int maxWidth, float t, int x, int y, const char *text);
    void renderSongInfo(SkCanvas &canvas, SkFont &font, SkFont &fontMinorInfo, bool smallMode, int maxWidth);
    CppLyrics(DataSource *dataSource);
    void render(SkCanvas *canvas, SkSurface *);
    void animate(const double deltaTime);
};