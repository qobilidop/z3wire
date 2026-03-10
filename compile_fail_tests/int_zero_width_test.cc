#include "z3wire/int.h"

// Int<0, false> should fail: "Bit-width must be between 1 and 64."
template class z3w::Int<0, false>;
