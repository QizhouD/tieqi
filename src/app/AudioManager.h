#pragma once

#include "app/Synth.h"

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Clock.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tq {

// Prefer a file with the same name (wav, ogg, flac). Otherwise use the synthesized cue.
class AudioManager {
  public:
    void loadDefaults(const std::filesystem::path &audioDir);
    void play(const std::string &name, float volume = 100.f, float pitch = 1.f);
    void setMasterVolume(float volume);
    float masterVolume() const { return master_; }
    void setMuted(bool muted);
    bool muted() const { return muted_; }
    bool has(const std::string &name) const { return buffers_.find(name) != buffers_.end(); }

  private:
    struct Voice {
        std::optional<sf::Sound> sound;
    };

    bool tryFile(const std::string &name, const std::filesystem::path &audioDir);
    Voice *acquire();

    std::unordered_map<std::string, sf::SoundBuffer> buffers_;
    std::vector<Voice> voices_ = std::vector<Voice>(8);
    float master_ = 70.f;
    bool muted_ = false;
    sf::Clock clock_;
    float lastCrack_ = -1.f;
};

} // namespace tq
