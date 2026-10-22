#include "runtime/fixed_math.h"

#include <cmath>
#include <cstdint>
#include <limits>

namespace mother {

namespace {

constexpr double kPi = 3.141592653589793;
constexpr double kHalfPi = 1.5707963267948966;
constexpr double kQuarterPi = 0.7853981633974483;
constexpr double kSixthPi = 0.5235987755982988;
constexpr double kTwelfthTan = 0.2679491924311227;  // tan(pi/12)
constexpr double kSqrt3 = 1.7320508075688772;

// pi/2 split so that k * kPio2Hi and k * kPio2Mid are exact for |k| < 2^20 (Cody-Waite).
constexpr double kInvHalfPi = 6.36619772367581382433e-01;
constexpr double kPio2Hi = 1.57079632673412561417e+00;
constexpr double kPio2Mid = 6.07710050630396597660e-11;
constexpr double kPio2Lo = 2.02226624879595063154e-21;

constexpr double kInvLn2 = 1.44269504088896338700e+00;
constexpr double kLn2Hi = 6.93147180369123816490e-01;
constexpr double kLn2Lo = 1.90821492927058770002e-10;

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

// Taylor series on |r| <= pi/4; the first omitted terms are below 1e-19.
double sin_kernel(double r) {
    const double z = r * r;
    const double p = -1.0 / 6.0 + z * (1.0 / 120.0 + z * (-1.0 / 5040.0 + z * (1.0 / 362880.0 +
                     z * (-1.0 / 39916800.0 + z * (1.0 / 6227020800.0 + z * (-1.0 / 1307674368000.0 +
                     z * (1.0 / 355687428096000.0)))))));
    return r + r * z * p;
}

double cos_kernel(double r) {
    const double z = r * r;
    const double p = -1.0 / 2.0 + z * (1.0 / 24.0 + z * (-1.0 / 720.0 + z * (1.0 / 40320.0 +
                     z * (-1.0 / 3628800.0 + z * (1.0 / 479001600.0 + z * (-1.0 / 87178291200.0 +
                     z * (1.0 / 20922789888000.0 + z * (-1.0 / 6402373705728000.0))))))));
    return 1.0 + z * p;
}

// Reduces x to r in [-pi/4, pi/4] with x = r + k * pi/2; returns k mod 4.
int reduce(double x, double& r) {
    const double k = std::floor(x * kInvHalfPi + 0.5);
    r = ((x - k * kPio2Hi) - k * kPio2Mid) - k * kPio2Lo;
    const double q = k - 4.0 * std::floor(k * 0.25);  // exact: 0, 1, 2 or 3
    return int(q);
}

// atan for 0 <= x <= 1.
double atan_unit(double x) {
    const bool shifted = x > kTwelfthTan;
    if (shifted) x = (x * kSqrt3 - 1.0) / (x + kSqrt3);  // atan(x) = pi/6 + atan(this)
    const double z = x * x;
    double p = 0.0;
    for (int n = 14; n >= 0; --n) p = (n % 2 == 0 ? 1.0 : -1.0) / double(2 * n + 1) + z * p;
    const double a = x * p;
    return shifted ? kSixthPi + a : a;
}

}  // namespace

double fixed_sin(double x) {
    if (!std::isfinite(x)) return kNaN;
    double r = 0.0;
    switch (reduce(x, r)) {
    case 0: return sin_kernel(r);
    case 1: return cos_kernel(r);
    case 2: return -sin_kernel(r);
    default: return -cos_kernel(r);
    }
}

double fixed_cos(double x) {
    if (!std::isfinite(x)) return kNaN;
    double r = 0.0;
    switch (reduce(x, r)) {
    case 0: return cos_kernel(r);
    case 1: return -sin_kernel(r);
    case 2: return -cos_kernel(r);
    default: return sin_kernel(r);
    }
}

double fixed_atan2(double y, double x) {
    if (std::isnan(x) || std::isnan(y)) return kNaN;
    if (x == 0.0 && y == 0.0) return 0.0;
    const double ax = std::fabs(x), ay = std::fabs(y);
    double a = 0.0;
    if (ax == ay) {
        a = kQuarterPi;  // also both infinite
    } else if (ay < ax) {
        a = atan_unit(ay / ax);
    } else {
        a = kHalfPi - atan_unit(ax / ay);
    }
    if (x < 0.0) a = kPi - a;
    return y < 0.0 ? -a : a;
}

double fixed_exp(double x) {
    if (std::isnan(x)) return x;
    if (x > 709.782712893384) return std::numeric_limits<double>::infinity();
    if (x < -745.1332191019411) return 0.0;
    const double k = std::floor(x * kInvLn2 + 0.5);
    const double r = (x - k * kLn2Hi) - k * kLn2Lo;  // |r| <= ln2 / 2
    double p = 0.0;
    double factorial = 6227020800.0;  // 13!
    for (int n = 13; n >= 1; --n) {
        p = (p + 1.0 / factorial) * r;
        factorial /= n;
    }
    return std::ldexp(1.0 + p, int(k));
}

}  // namespace mother
