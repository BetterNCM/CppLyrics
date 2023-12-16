#pragma once
#include "../pch.h"
#include "LyricParser.h"
#include "core/SkImage.h"

template<typename T>
using AtomRC = std::atomic<std::shared_ptr<T>>;
class DataSource {
    std::mutex mutex;
    AtomRC<std::vector<LyricLine>> lines{};
    float currentTimeExt = -1.f;
    std::atomic<bool> isPaused = false;
    AtomRC<std::string> songName{};
    AtomRC<std::string> songArtist{};
    AtomRC<std::array<float, 3>> songColor1;
    AtomRC<std::array<float, 3>> songColor2;
    sk_sp<SkImage> songCover = nullptr;

    std::vector<std::function<void(float)>> currentTimeCallbacks;

public:
    bool isFull() {
        return lines.load() != nullptr && !lines.load()->empty() && songName.load() != nullptr &&
               songArtist.load() != nullptr && songColor1.load() != nullptr && songColor2.load() != nullptr &&
               songCover != nullptr;
    }

    void setLyrics(const std::string &lyrics) {
        lines.store(std::make_shared<std::vector<LyricLine>>(LyricParser::parse(lyrics)));
    }

    void setSongInfo(const std::string &&name, const std::string &&artist) {
        songName = std::make_shared<std::string>(name);
        songArtist = std::make_shared<std::string>(artist);
    }

    void setSongColor(const std::array<float, 3> color1, const std::array<float, 3> color2) {
        songColor1 = std::make_shared<std::array<float, 3>>(color1);
        songColor2 = std::make_shared<std::array<float, 3>>(color2);
    }

    void setSongCover(const sk_sp<SkImage> cover) {
        songCover = cover;
    }

    void setPaused(bool paused) {
        isPaused = paused;
    }

    bool getPaused() {
        return isPaused;
    }

    std::shared_ptr<std::vector<LyricLine> const> getLines() {
        return lines.load();
    }

    std::shared_ptr<std::string> getSongName() {
        return songName.load();
    }

    std::shared_ptr<std::string> getSongArtist() {
        return songArtist.load();
    }

    std::shared_ptr<std::array<float, 3>> getSongColor1() {
        return songColor1.load();
    }

    std::shared_ptr<std::array<float, 3>> getSongColor2() {
        std::lock_guard<std::mutex> lock(mutex);
        return songColor2.load();
    }

    sk_sp<SkImage> getSongCover() {
        return songCover;
    }

    void addCurrentTimeCallback(std::function<void(float)> callback) {
        std::lock_guard<std::mutex> lock(mutex);
        currentTimeCallbacks.push_back(callback);
    }

    void setCurrentTime(float time) {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto &callback: currentTimeCallbacks) {
            callback(time);
        }
        currentTimeExt = time;
    }
};
