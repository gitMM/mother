// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#pragma once

#include <math.h>

// MOTHER patch: deterministic replacement for pow(), used by the ^ operator in the VM and by
// the compiler's constant folding. libm pow() is not correctly rounded and its results differ
// between platforms, which would break MOTHER's cross-platform determinism.
//   - integer exponents (|b| <= 2^53): exponentiation by squaring, a fixed sequence of IEEE
//     multiplies (b == 2 and b == 3 give exactly the VM's a*a and a*a*a fast paths);
//   - b == 0.5: sqrt(a), matching the VM's fast path;
//   - any other exponent: NaN, since MOTHER has no fixed-algorithm exp/log to build it from.
inline double luau_mother_pow(double a, double b)
{
    if (b >= -9007199254740992.0 && b <= 9007199254740992.0 && b == floor(b))
    {
        unsigned long long e = (unsigned long long)(b < 0 ? -b : b);
        double result = 1.0;
        double base = a;
        while (e != 0)
        {
            if (e & 1)
                result *= base;
            e >>= 1;
            if (e != 0)
                base *= base;
        }
        return b < 0 ? 1.0 / result : result;
    }

    if (b == 0.5)
        return sqrt(a);

    return NAN;
}
