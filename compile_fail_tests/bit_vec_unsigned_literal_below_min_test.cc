#include "z3wire/bit_vec.h"

// UInt<8> min is 0; Literal<-1> should fail.
auto x = z3w::UInt<8>::Literal<-1>();
