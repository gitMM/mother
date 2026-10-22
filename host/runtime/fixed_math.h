#pragma once

namespace mother {

// The cart's transcendental functions (brief section 4): fixed algorithms built only from
// + - * / and exact operations (floor, ldexp), compiled with strict floating point, so every
// platform returns the same bits. Accurate to within a few units in the last place for
// ordinary arguments. Angles are in radians.
double fixed_sin(double x);
double fixed_cos(double x);
double fixed_atan2(double y, double x);  // atan2(0, 0) is 0
double fixed_exp(double x);

}  // namespace mother
