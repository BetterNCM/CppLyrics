//
// Created by MicroBlock on 2023/10/15.
//

#ifndef CORE_UTILS_H
#define CORE_UTILS_H
#include "../pch.h"

class Utils {
public:
    static std::vector<std::string> split(const std::string &str, char delimiter) {
        auto result = std::vector<std::string>();
        auto start = 0;
        auto end = 0;
        while ((end = str.find(delimiter, start)) != std::string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + 1;
        }
        result.push_back(str.substr(start));
        return result;
    }
    static bool isPureAscii(const std::string &str) {
        return std::all_of(str.begin(), str.end(), [](unsigned char c) { return c < 128; });
    }
};


#endif//CORE_UTILS_H
