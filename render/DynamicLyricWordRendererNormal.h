//
// Created by MicroBlock on 2023/10/15.
//

#ifndef CORE_DYNAMICLYRICWORDRENDERERNORMAL_H
#define CORE_DYNAMICLYRICWORDRENDERERNORMAL_H

#include "../Lyric.h"
#include "../pch.h"
#include "./DynamicLyricWordRenderer.h"

struct DynamicLyricWordRendererNormal : public DynamicLyricWordRenderer {
    float renderLyricWord(
            SkCanvas *canvas,
            float relativeTime,// relative to the lyric line
            const LyricDynamicWord &word,
            float x, float y,
            const SkFont &font,
            float blur, float opacity) const override {
        relativeTime = std::clamp(relativeTime - word.start, 0.f, word.end - word.start);
        float progress = (relativeTime) / (word.end - word.start);

        // draw from top to bottom (change baseline to top)
        y += font.getSize();// - (1 - progress) * 4.f;

        SkPaint paintBg, paint1;
        paintBg.setAntiAlias(true);

        //  background light
        //        paintBg.setColor(SkColorSetARGB(30, 183, 201, 217));
        //        paintBg.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 10.f, true));
        //        canvas->drawString(word.word.c_str(), x + 2.f, y + 3.f, font, paintBg);

        if (blur > 0.f) {
            paintBg.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blur, blur));
            paint1.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blur, blur));
        } else {
            paintBg.setMaskFilter(nullptr);
        }

        // paint background text
        paintBg.setBlendMode(SkBlendMode::kPlus);
        paintBg.setColor(SkColorSetARGB(90 * opacity, 255, 255, 255));

        sk_sp<SkTextBlob> textBlob = SkTextBlob::MakeFromString(word.word.c_str(), font);

        canvas->drawTextBlob(textBlob, x, y, paintBg);

        // paint dynamic word progress
        const auto textWidth = font.measureText(word.word.c_str(), word.word.size(), SkTextEncoding::kUTF8);
        // set mask texture
        SkPoint pts[2] = {{x, 0},
                          {x + textWidth, 0}};
        SkColor colors[2] = {SkColorSetARGB(0xFF * opacity, 0xFF, 0xFF, 0xFF), SkColorSetARGB(0x0, 0xFF, 0xFF, 0xFF)};
        const auto translate = SkMatrix::Translate(textWidth * (progress * 2 - 1), 0);
        const auto shader = SkGradientShader::MakeLinear(pts,
                                                         colors,
                                                         nullptr, 2, SkTileMode::kClamp, 0, &translate);


        paint1.setAntiAlias(true);
        paint1.setShader(shader);
        canvas->drawTextBlob(textBlob, x, y, paint1);

        return font.measureText(word.word.c_str(), word.word.size(), SkTextEncoding::kUTF8);
    }

    [[nodiscard]] float measureLyricWord(const LyricDynamicWord &word, const SkFont &font) const override {
        return font.measureText(word.word.c_str(), word.word.size(), SkTextEncoding::kUTF8);
    }
};

#endif//CORE_DYNAMICLYRICWORDRENDERERNORMAL_H
