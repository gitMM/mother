#pragma once

#include <cstdint>
#include <cstring>

namespace mother {

// FNV-1a 64 over little-endian bytes, so a hash is the same on every platform.
class Fnv64 {
public:
    void byte(uint8_t b) { h_ = (h_ ^ b) * 1099511628211ull; }
    void u16(uint16_t v) { byte(uint8_t(v)), byte(uint8_t(v >> 8)); }
    void u32(uint32_t v) { u16(uint16_t(v)), u16(uint16_t(v >> 16)); }
    void u64(uint64_t v) { u32(uint32_t(v)), u32(uint32_t(v >> 32)); }

    // A double by its IEEE bits, with every NaN folded into one value and -0 into 0.
    void number(double v) {
        uint64_t bits = 0x7FF8000000000000ull;
        if (v == v) {
            if (v == 0.0) v = 0.0;
            std::memcpy(&bits, &v, sizeof bits);
        }
        u64(bits);
    }

    uint64_t value() const { return h_; }

private:
    uint64_t h_ = 14695981039346656037ull;
};

// splitmix64's finalizer: a strong 64-bit mix, used for Zobrist keys and seeding.
inline uint64_t mix64(uint64_t z) {
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

}  // namespace mother
