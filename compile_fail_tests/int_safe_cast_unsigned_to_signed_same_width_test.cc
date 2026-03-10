#include "z3wire/int.h"

// safe_cast UInt<8> -> SInt<8> should fail: needs target width > source.
auto x = z3w::safe_cast<z3w::SInt<8>>(z3w::UInt<8>(0));
