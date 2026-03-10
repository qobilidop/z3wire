#include "z3wire/int.h"

// safe_cast SInt<8> -> SInt<4> should fail: target too narrow.
auto x = z3w::safe_cast<z3w::SInt<4>>(z3w::SInt<8>(0));
