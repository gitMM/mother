#include "runtime/rng.h"

#include "runtime/hash.h"

namespace mother {

namespace {

uint32_t rotl(uint32_t x, int k) {
    return (x << k) | (x >> (32 - k));
}

}  // namespace

Rng::Rng(uint64_t seed) {
    uint64_t z = seed;
    for (size_t i = 0; i < s_.size(); i += 2) {
        z += 0x9E3779B97F4A7C15ull;
        const uint64_t v = mix64(z);
        s_[i] = uint32_t(v);
        s_[i + 1] = uint32_t(v >> 32);
    }
}

uint32_t Rng::next() {
    const uint32_t result = rotl(s_[1] * 5, 7) * 9;
    const uint32_t t = s_[1] << 9;
    s_[2] ^= s_[0];
    s_[3] ^= s_[1];
    s_[1] ^= s_[2];
    s_[0] ^= s_[3];
    s_[2] ^= t;
    s_[3] = rotl(s_[3], 11);
    return result;
}

double Rng::unit() {
    return next() * (1.0 / 4294967296.0);
}

int64_t Rng::range(int64_t lo, int64_t hi) {
    const uint64_t span = uint64_t(hi - lo) + 1;
    if (span > 0xFFFFFFFFull) return lo + int64_t(next());  // the full 2^32 range
    // Reject draws from the incomplete last block so every value is equally likely.
    const uint64_t limit = (0x100000000ull / span) * span;
    uint64_t x = next();
    while (x >= limit) x = next();
    return lo + int64_t(x % span);
}

}  // namespace mother
