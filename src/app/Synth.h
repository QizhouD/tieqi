#pragma once

#include <cstdint>
#include <vector>

namespace tq {

inline constexpr unsigned kSampleRate = 44100;
inline constexpr float kCrackGapSec = 0.09f;

struct SynthSet {
    std::vector<std::int16_t> thud;
    std::vector<std::int16_t> crack;
    std::vector<std::int16_t> drum;
    std::vector<std::int16_t> rout;
};

// Four cues synthesized like the prototype. Noise uses its own fixed seed and does not touch tq::Rng.
SynthSet synthesizeSfx(unsigned sampleRate = kSampleRate);

// Two cracks are at least 90 ms apart. A negative lastSec means the cue has not played.
bool allowCrack(float nowSec, float &lastSec);

} // namespace tq
