#include "core/sim/Rng.h"

namespace tq {

Rng::Rng(std::uint64_t seed) : state_(0), increment_((1ULL << 1) | 1ULL) {
    next();
    state_ += seed;
    next();
}

std::uint32_t Rng::next() {
    const std::uint64_t old = state_;
    state_ = old * 6364136223846793005ULL + increment_;
    const std::uint32_t xorshifted = static_cast<std::uint32_t>(((old >> 18U) ^ old) >> 27U);
    const std::uint32_t rotation = static_cast<std::uint32_t>(old >> 59U);
    return (xorshifted >> rotation) | (xorshifted << ((32U - rotation) & 31U));
}

float Rng::uniform() {
    // Divide the 24 bits in double precision so the maximum does not round to 1 in float.
    return static_cast<float>(static_cast<double>(next() >> 8U) / 16777216.0);
}

} // namespace tq
