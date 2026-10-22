#pragma once

#include <array>
#include <cstdint>

namespace mother {

// xoshiro128** (Blackman and Vigna), seeded through splitmix64. Integer-only, so every
// platform draws the same numbers from the same seed.
class Rng {
public:
    explicit Rng(uint64_t seed = 0);

    uint32_t next();
    double unit();                           // [0, 1) in steps of 2^-32
    int64_t range(int64_t lo, int64_t hi);   // uniform in [lo, hi]; hi - lo < 2^32; no modulo bias

    const std::array<uint32_t, 4>& state() const { return s_; }

private:
    std::array<uint32_t, 4> s_{};
};

}  // namespace mother
