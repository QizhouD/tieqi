#pragma once

#include <cstdint>

namespace tq {

// PCG32 XSH RR。uniform() 取高 24 位，结果在 [0, 1)。
class Rng {
  public:
    explicit Rng(std::uint64_t seed);

    std::uint32_t next();
    float uniform();

  private:
    std::uint64_t state_ = 0;
    std::uint64_t increment_ = 1;
};

} // namespace tq
