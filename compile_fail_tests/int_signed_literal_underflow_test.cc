#include "z3wire/int.h"

// SInt<8> min is -128; Literal<-129> should fail.
auto x = z3w::SInt<8>::Literal<-129>();
