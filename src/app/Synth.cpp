#include "app/Synth.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>

namespace tq {
namespace {

constexpr float kPi = 3.14159265358979323846f;

struct Biquad {
    float b0 = 1.f;
    float b1 = 0.f;
    float b2 = 0.f;
    float a1 = 0.f;
    float a2 = 0.f;
    float x1 = 0.f;
    float x2 = 0.f;
    float y1 = 0.f;
    float y2 = 0.f;

    void lowpass(float freq, float rate) { set(freq, rate, false, false); }
    void highpass(float freq, float rate) { set(freq, rate, true, false); }
    void bandpass(float freq, float rate) { set(freq, rate, false, true); }

    float process(float x) {
        const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }

    void set(float freq, float rate, bool high, bool band) {
        const float q = 0.707f;
        const float w0 = 2.f * kPi * freq / rate;
        const float cw = std::cos(w0);
        const float sw = std::sin(w0);
        const float alpha = sw / (2.f * q);
        const float a0 = 1.f + alpha;
        if (band) {
            b0 = alpha / a0;
            b1 = 0.f;
            b2 = -alpha / a0;
        } else if (high) {
            b0 = ((1.f + cw) * 0.5f) / a0;
            b1 = -(1.f + cw) / a0;
            b2 = b0;
        } else {
            b0 = ((1.f - cw) * 0.5f) / a0;
            b1 = (1.f - cw) / a0;
            b2 = b0;
        }
        a1 = (-2.f * cw) / a0;
        a2 = (1.f - alpha) / a0;
    }
};

struct Mix {
    std::vector<float> samples;

    void add(std::size_t offset, float value) {
        if (offset >= samples.size()) {
            samples.resize(offset + 1, 0.f);
        }
        samples[offset] += value;
    }
};

float envelope(float t, float dur, float gain) {
    if (!(dur > 0.f) || !(gain > 0.f)) {
        return 0.f;
    }
    const float u = std::clamp(t / dur, 0.f, 1.f);
    return gain * std::pow(0.0008f / gain, u);
}

void addTone(Mix &mix, std::size_t offset, float freq, float dur, float gain, unsigned rate) {
    const std::size_t count = static_cast<std::size_t>(std::lround(static_cast<double>(dur) * rate));
    const float endFreq = freq * 0.55f;
    float phase = 0.f;
    for (std::size_t i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(rate);
        const float u = count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1) : 0.f;
        const float f = freq * std::pow(endFreq / freq, u);
        phase += 2.f * kPi * f / static_cast<float>(rate);
        mix.add(offset + i, std::sin(phase) * envelope(t, dur, gain));
    }
}

void addNoise(Mix &mix, std::size_t offset, float dur, float gain, Biquad filter, unsigned rate, std::mt19937 &rng) {
    std::uniform_real_distribution<float> white(-1.f, 1.f);
    const std::size_t count = static_cast<std::size_t>(std::lround(static_cast<double>(dur) * rate));
    for (std::size_t i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(rate);
        mix.add(offset + i, filter.process(white(rng)) * envelope(t, dur, gain));
    }
}

std::vector<std::int16_t> toPcm(const Mix &mix) {
    std::vector<std::int16_t> pcm(mix.samples.size());
    for (std::size_t i = 0; i < mix.samples.size(); ++i) {
        const float clamped = std::clamp(mix.samples[i], -1.f, 1.f);
        pcm[i] = static_cast<std::int16_t>(std::lround(clamped * 32767.f));
    }
    return pcm;
}

} // namespace

SynthSet synthesizeSfx(unsigned sampleRate) {
    std::mt19937 rng(1593u);
    SynthSet set;

    {
        Mix mix;
        Biquad filter;
        filter.lowpass(220.f, static_cast<float>(sampleRate));
        addNoise(mix, 0, 0.45f, 0.55f, filter, sampleRate, rng);
        addTone(mix, 0, 70.f, 0.35f, 0.5f, sampleRate);
        set.thud = toPcm(mix);
    }
    {
        Mix mix;
        Biquad filter;
        filter.highpass(1800.f, static_cast<float>(sampleRate));
        addNoise(mix, 0, 0.16f, 0.14f, filter, sampleRate, rng);
        set.crack = toPcm(mix);
    }
    {
        Mix mix;
        const std::size_t rate = sampleRate;
        addTone(mix, 0, 82.f, 0.5f, 0.6f, sampleRate);
        addTone(mix, static_cast<std::size_t>(std::lround(0.38 * rate)), 82.f, 0.5f, 0.6f, sampleRate);
        addTone(mix, static_cast<std::size_t>(std::lround(0.76 * rate)), 64.f, 0.75f, 0.7f, sampleRate);
        set.drum = toPcm(mix);
    }
    {
        Mix mix;
        Biquad filter;
        filter.bandpass(700.f, static_cast<float>(sampleRate));
        addNoise(mix, 0, 0.6f, 0.12f, filter, sampleRate, rng);
        set.rout = toPcm(mix);
    }
    return set;
}

bool allowCrack(float nowSec, float &lastSec) {
    if (lastSec >= 0.f && nowSec - lastSec < kCrackGapSec) {
        return false;
    }
    lastSec = nowSec;
    return true;
}

} // namespace tq
