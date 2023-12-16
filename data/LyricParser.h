//
// Created by MicroBlock on 2023/10/15.
//

#ifndef CORE_LYRICPARSER_H
#define CORE_LYRICPARSER_H

#include "../Lyric.h"
#include <string>
#include <vector>


/**
 * 歌词解析器
 *
 * 歌词格式（动态）：
 * (绝对毫秒时间:绝对毫秒结束时间)词`(绝对毫秒时间:绝对毫秒结束时间)词`(绝对毫秒时间:绝对毫秒结束时间)词[|第二歌词|第三歌词]
 * (绝对毫秒时间:绝对毫秒结束时间)词`(绝对毫秒时间:绝对毫秒结束时间)词`(绝对毫秒时间:绝对毫秒结束时间)词[|第二歌词|第三歌词]
 *
 * 歌词格式（静态）：
 * (绝对毫秒时间:绝对毫秒结束时间)句子
 */
class LyricParser {
public:
    static std::vector<LyricLine> parse(const std::string &lyric);
};


#endif//CORE_LYRICPARSER_H
