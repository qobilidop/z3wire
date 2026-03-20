#include "z3wire/bit_vec.h"

// SInt<8> max is 127; Literal<128> should fail.
auto x = z3w::SInt<8>::Literal<128>();
