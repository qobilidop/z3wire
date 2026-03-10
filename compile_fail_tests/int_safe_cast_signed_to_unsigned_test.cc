#include "z3wire/int.h"

// safe_cast from signed to unsigned is always forbidden.
auto x = z3w::safe_cast<z3w::UInt<8>>(z3w::SInt<8>(0));
