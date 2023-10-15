//
// Created by MicroBlock on 2023/10/15.
//

#ifndef CORE_LYRIC_H
#define CORE_LYRIC_H

#include <string>
#include <variant>
#include <vector>

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
    std::vector<std::string> subLyrics;
};


#endif//CORE_LYRIC_H
