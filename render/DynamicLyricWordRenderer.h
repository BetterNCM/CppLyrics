//
// Created by MicroBlock on 2023/10/15.
//

#ifndef CORE_DYNAMICLYRICWORDRENDERER_H
#define CORE_DYNAMICLYRICWORDRENDERER_H

#include "../Lyric.h"
#include "../pch.h"

struct DynamicLyricWordRenderer {
    virtual float renderLyricWord(SkCanvas *canvas,
                                  float relativeTime,
                                  const LyricDynamicWord &word,
                                  float x, float y,
                                  const SkFont &font, float blur) const = 0;
    [[nodiscard]] virtual float measureLyricWord(const LyricDynamicWord &word, const SkFont &font) const = 0;
};

#endif//CORE_DYNAMICLYRICWORDRENDERER_H
