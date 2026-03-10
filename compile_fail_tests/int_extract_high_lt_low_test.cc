#include "z3wire/int.h"

// extract<0, 3> should fail: High (0) must be >= Low (3).
auto x = z3w::extract<0, 3>(z3w::UInt<8>(0));
