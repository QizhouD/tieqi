#include "app/AudioManager.h"

#include <SFML/Audio/SoundChannel.hpp>
#include <SFML/System/Time.hpp>

#include <algorithm>
#include <vector>

namespace tq {

void AudioManager::loadDefaults(const std::filesystem::path &audioDir) {
    const SynthSet synth = synthesizeSfx();
    const auto add = [this](const std::string &name, const std::vector<std::int16_t> &samples) {
        sf::SoundBuffer buffer;
        const std::vector<sf::SoundChannel> channels{sf::SoundChannel::Mono};
        if (!buffer.loadFromSamples(samples.data(), samples.size(), 1, kSampleRate, channels)) {
            return;
        }
        buffers_[name] = std::move(buffer);
    };
    if (!tryFile("thud", audioDir)) {
        add("thud", synth.thud);
    }
    if (!tryFile("crack", audioDir)) {
        add("crack", synth.crack);
    }
    if (!tryFile("drum", audioDir)) {
        add("drum", synth.drum);
    }
    if (!tryFile("rout", audioDir)) {
        add("rout", synth.rout);
    }
}

bool AudioManager::tryFile(const std::string &name, const std::filesystem::path &audioDir) {
    if (audioDir.empty()) {
        return false;
    }
    for (const char *extension : {".wav", ".ogg", ".flac"}) {
        const std::filesystem::path file = audioDir / (name + extension);
        std::error_code error;
        if (!std::filesystem::exists(file, error)) {
            continue;
        }
        sf::SoundBuffer buffer;
        if (!buffer.loadFromFile(file)) {
            continue;
        }
        buffers_[name] = std::move(buffer);
        return true;
    }
    return false;
}

void AudioManager::play(const std::string &name, float volume, float pitch) {
    if (muted_) {
        return;
    }
    if (name == "crack" && !allowCrack(clock_.getElapsedTime().asSeconds(), lastCrack_)) {
        return;
    }
    const auto found = buffers_.find(name);
    if (found == buffers_.end()) {
        return;
    }
    Voice *voice = acquire();
    if (voice == nullptr) {
        return;
    }
    voice->sound.emplace(found->second);
    voice->sound->setVolume(volume * master_ / 100.f);
    voice->sound->setPitch(pitch);
    voice->sound->play();
}

AudioManager::Voice *AudioManager::acquire() {
    for (Voice &voice : voices_) {
        if (!voice.sound || voice.sound->getStatus() != sf::SoundSource::Status::Playing) {
            return &voice;
        }
    }
    return voices_.empty() ? nullptr : &voices_.front();
}

void AudioManager::setMasterVolume(float volume) { master_ = std::clamp(volume, 0.f, 100.f); }

void AudioManager::setMuted(bool muted) {
    muted_ = muted;
    if (!muted_) {
        return;
    }
    for (Voice &voice : voices_) {
        if (voice.sound) {
            voice.sound->stop();
        }
    }
}

} // namespace tq
