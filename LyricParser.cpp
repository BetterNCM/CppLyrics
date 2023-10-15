//
// Created by MicroBlock on 2023/10/15.
//

#include "LyricParser.h"
#include "Utils.h"
std::vector<LyricLine> LyricParser::parse(const std::string &lyric) {
    auto lines = std::vector<LyricLine>();

    auto lyricLines = Utils::split(lyric, '\n');

    for (const auto &lyricLine: lyricLines) {
        auto lineParts = Utils::split(lyricLine, '|');
        auto line = LyricLine{
                .lyric = std::vector<LyricDynamicWord>(),
        };
        // (time ms:time ms end)word`(time ms:time ms end)word`(time ms:time ms end)word

        auto lyricWord = LyricDynamicWord{};
        char state = 'l';// l: (, r: ), t: timeStart, e: timeEnd, w: word
        std::string word;
        std::string timeStart;
        std::string timeEnd;

        float lineStartTime = -1;

        line.lyric = std::vector<LyricDynamicWord>();

        lineParts[0] += '`';
        for (const auto &c: lineParts[0]) {
            switch (state) {
                case 'l':
                    if (c == '(') {
                        state = 't';
                    }
                    break;
                case 't':
                    if (c == ':') {
                        state = 'e';
                    } else {
                        timeStart += c;
                    }
                    break;
                case 'e':
                    if (c == ')') {
                        state = 'w';
                    } else {
                        timeEnd += c;
                    }
                    break;
                case 'w':
                    if (c == '`') {
                        if (lineStartTime == -1) {
                            lineStartTime = std::stof(timeStart);
                        }
                        state = 'l';
                        lyricWord.word = word;
                        lyricWord.start = std::stof(timeStart) - lineStartTime;
                        lyricWord.end = std::stof(timeEnd) - lineStartTime;

                        word = "";
                        timeStart = "";
                        timeEnd = "";

                        std::get<0>(line.lyric).emplace_back(lyricWord);
                    } else {
                        word += c;
                    }
                    break;
            }
        }

        line.start = lineStartTime;
        line.end = lineStartTime + std::get<0>(line.lyric).back().end;
        line.isDynamic = true;
        line.subLyrics = std::vector<std::string>(lineParts.begin() + 1, lineParts.end());

        lines.push_back(line);
    }

    return lines;
}
