//
// Created by MicroBlock on 2023/10/15.
//

#ifndef CORE_RENDERTEXTWRAP_H
#define CORE_RENDERTEXTWRAP_H

#include "../external/utf8.h"
#include "../pch.h"
#include <iterator>


float renderTextWithWrap(SkCanvas &canvas, const SkPaint &paint, const SkFont &font, const SkFont &layoutFont, float maxWidth, float x, float y, const std::string &text) {
    float dx = 0, dy = font.getSize();

    utf8_iter iter;
    utf8_init(&iter, text.c_str());
    while (utf8_next(&iter)) {
        const char *c = utf8_getchar(&iter);
        SkRect bounds;
        layoutFont.measureText(c, strlen(c), SkTextEncoding::kUTF8, &bounds);
        SkRect boundsNormal;
        font.measureText(c, strlen(c), SkTextEncoding::kUTF8, &boundsNormal);
        if (dx + bounds.width() > maxWidth) {
            dx = 0;
            dy += font.getSize();
        }
        canvas.drawSimpleText(c, strlen(c), SkTextEncoding::kUTF8, x + dx, y + dy, font, paint);
        dx += boundsNormal.width() * 1.1;
    }

    return dy + font.getSize();
}

float measureTextWithWrap(const SkFont &font, const SkFont &layoutFont, float maxWidth, const std::string &text) {
    float dx = 0, dy = font.getSize();

    utf8_iter iter;
    utf8_init(&iter, text.c_str());
    while (utf8_next(&iter)) {
        const char *c = utf8_getchar(&iter);
        SkRect bounds;
        layoutFont.measureText(c, strlen(c), SkTextEncoding::kUTF8, &bounds);
        SkRect boundsNormal;
        font.measureText(c, strlen(c), SkTextEncoding::kUTF8, &boundsNormal);
        if (dx + bounds.width() > maxWidth) {
            dx = 0;
            dy += font.getSize();
        }
        dx += boundsNormal.width() * 1.1;
    }

    return dy + font.getSize();
}

#endif//CORE_RENDERTEXTWRAP_H
