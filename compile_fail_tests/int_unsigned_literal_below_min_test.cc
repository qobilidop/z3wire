#include "z3wire/int.h"

// UInt<8> min is 0; Literal<-1> should fail.
auto x = z3w::UInt<8>::Literal<-1>();
