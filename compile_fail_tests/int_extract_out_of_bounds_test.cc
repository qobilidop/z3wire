#include "z3wire/int.h"

// extract<8, 0> on UInt<8> should fail: High (8) must be < width (8).
auto x = z3w::extract<8, 0>(z3w::UInt<8>(0));
