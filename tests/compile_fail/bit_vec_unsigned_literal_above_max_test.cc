#include "z3wire/bit_vec.h"

// UInt<8> max is 255; Literal<256> should fail.
auto x = z3w::UInt<8>::Literal<256>();
