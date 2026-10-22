#include <cmath>
#include <cstring>
#include <limits>

#include "runtime/fixed_math.h"
#include "test.h"

using namespace mother;

namespace {

bool close(double a, double b, double tolerance = 4e-16) {
    return std::fabs(a - b) <= tolerance * (1.0 + std::fabs(b));
}

uint64_t bits(double v) {
    uint64_t b = 0;
    std::memcpy(&b, &v, sizeof b);
    return b;
}

}  // namespace

TEST(fixedmath_sin_and_cos_match_libm) {
    int bad = 0;
    for (int i = -20000; i <= 20000; ++i) {
        const double x = i * 0.0031;  // about -62 .. 62 radians
        bad += close(fixed_sin(x), std::sin(x)) && close(fixed_cos(x), std::cos(x)) ? 0 : 1;
    }
    CHECK_EQ(bad, 0);
    CHECK_EQ(fixed_sin(0.0), 0.0);
    CHECK_EQ(fixed_cos(0.0), 1.0);
    CHECK(close(fixed_sin(1e5), std::sin(1e5), 1e-11));
    CHECK(std::isnan(fixed_sin(std::numeric_limits<double>::infinity())));
    CHECK(std::isnan(fixed_cos(std::nan(""))));
}

TEST(fixedmath_atan2_matches_libm) {
    int bad = 0;
    for (int i = -60; i <= 60; ++i) {
        for (int j = -60; j <= 60; ++j) {
            const double y = i * 0.37, x = j * 0.29;
            if (x == 0.0 && y == 0.0) continue;
            bad += close(fixed_atan2(y, x), std::atan2(y, x)) ? 0 : 1;
        }
    }
    CHECK_EQ(bad, 0);
    CHECK_EQ(fixed_atan2(0.0, 0.0), 0.0);
    CHECK(close(fixed_atan2(1.0, 1.0), 0.7853981633974483));
    CHECK(close(fixed_atan2(0.0, -1.0), 3.141592653589793));
    CHECK(close(fixed_atan2(-1.0, 0.0), -1.5707963267948966));
    const double inf = std::numeric_limits<double>::infinity();
    CHECK(close(fixed_atan2(inf, inf), 0.7853981633974483));
}

TEST(fixedmath_exp_matches_libm) {
    int bad = 0;
    for (int i = -7000; i <= 7000; ++i) {
        const double x = i * 0.1;
        bad += close(fixed_exp(x), std::exp(x)) ? 0 : 1;
    }
    CHECK_EQ(bad, 0);
    CHECK_EQ(fixed_exp(0.0), 1.0);
    CHECK(std::isinf(fixed_exp(710.0)));
    CHECK_EQ(fixed_exp(-746.0), 0.0);
}

TEST(fixedmath_results_are_pinned) {
    // The exact bits every platform must produce, computed by an independent Python port of the
    // same algorithms. A change here breaks every recorded trace.
    CHECK_EQ(bits(fixed_sin(1.0)), 0x3feaed548f090ceeull);
    CHECK_EQ(bits(fixed_cos(2.5)), 0xbfe9a2f7ef858b7dull);
    CHECK_EQ(bits(fixed_atan2(-0.3, -0.7)), 0xc005e4c36ca0118aull);
    CHECK_EQ(bits(fixed_exp(3.7)), 0x4044394144eeec81ull);
}
