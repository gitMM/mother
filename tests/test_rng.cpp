#include <array>

#include "runtime/rng.h"
#include "test.h"

using namespace mother;

TEST(rng_same_seed_same_numbers) {
    Rng a(42), b(42), c(43);
    bool differs = false;
    for (int i = 0; i < 100; ++i) {
        const uint32_t x = a.next();
        CHECK_EQ(x, b.next());
        differs = differs || x != c.next();
    }
    CHECK(differs);
}

TEST(rng_sequence_is_pinned) {
    // xoshiro128** from splitmix64(0), checked against an independent Python implementation.
    // Any change here breaks every recorded trace.
    Rng r(0);
    const uint32_t first[4] = {r.next(), r.next(), r.next(), r.next()};
    CHECK_EQ(first[0], 0xdec9045du);
    CHECK_EQ(first[1], 0x9a089d75u);
    CHECK_EQ(first[2], 0xab77d362u);
    CHECK_EQ(first[3], 0xc3e16405u);
}

TEST(rng_unit_stays_in_range) {
    Rng r(1);
    double lo = 1.0, hi = 0.0;
    for (int i = 0; i < 10000; ++i) {
        const double u = r.unit();
        lo = u < lo ? u : lo;
        hi = u > hi ? u : hi;
    }
    CHECK(lo >= 0.0 && lo < 0.01);
    CHECK(hi < 1.0 && hi > 0.99);
}

TEST(rng_range_is_inclusive_and_even) {
    Rng r(2);
    std::array<int, 6> counts{};
    for (int i = 0; i < 60000; ++i) {
        const int64_t v = r.range(1, 6);
        CHECK(v >= 1 && v <= 6);
        if (v >= 1 && v <= 6) ++counts[size_t(v - 1)];
    }
    for (int c : counts) CHECK(c > 9000 && c < 11000);
    CHECK_EQ(r.range(5, 5), 5);
    const int64_t full = r.range(-2147483648ll, 2147483647ll);
    CHECK(full >= -2147483648ll && full <= 2147483647ll);
}
