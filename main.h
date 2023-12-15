#pragma once
#include "Lyric.h"
#include "LyricParser.h"
#include "pch.h"


static const bool enableLowEffect = false;


#define drawHorizontalLineAt(y, color)               \
    {                                                \
        SkPaint ___paint;                            \
        ___paint.setColor(SkColors::color);          \
        ___paint.setStrokeWidth(1.f);                \
        canvas->drawLine(0, y, kWidth, y, ___paint); \
    }

// TODO: thread safety is not guaranteed, refactor later


class CppLyrics {


public:
    ~CppLyrics();
    CppLyrics(const CppLyrics &cppLyrics) = delete;

    int kWidth = 630;
    int kHeight = 900;
    int lastFPS = 0;
    bool showTips = true;

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

    constexpr static const float marginBottom = 0.f;

    float renderLyricLine(
            SkCanvas *canvas,
            float relativeTime,
            const LyricLine &line,
            float x, float y,
            float maxWidth,
            const SkFont &font,
            const SkFont &layoutFont,
            const SkFont &fontSubLyrics,
            float blur = 0.f);

    float estimateLyricLineHeight(
            const LyricLine &line,
            float maxWidth,
            const SkFont &font,
            const SkFont &layoutFont,
            const SkFont &fontSubLyrics);

    float t = 0.f;
    float fluidTime = 0.f;
    bool useFluentBg = true;
    bool useFontBlur = false;
    bool useTextResize = false;
    void renderScrollingString(SkCanvas &canvas, SkFont &font, SkPaint &paint, int maxWidth, float t, int x, int y, const char *text);
    void renderSongInfo(SkCanvas &canvas, SkFont &font, SkFont &fontMinorInfo, bool smallMode, int maxWidth);
    CppLyrics();
    void render(SkCanvas *canvas);
    void animate(const double deltaTime);
};