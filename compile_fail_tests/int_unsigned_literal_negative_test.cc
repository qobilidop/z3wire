#include "z3wire/int.h"

// Unsigned literal must be non-negative; Literal<-1> should fail.
auto x = z3w::UInt<8>::Literal<-1>();
