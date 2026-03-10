#include "z3wire/int.h"

// safe_cast UInt<8> -> UInt<4> should fail: target too narrow.
auto x = z3w::safe_cast<z3w::UInt<4>>(z3w::UInt<8>(0));
