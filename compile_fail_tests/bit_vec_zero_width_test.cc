#include "z3wire/bit_vec.h"

// BitVec<0, false> should fail: "Bit-width must be at least 1."
template class z3w::BitVec<0, false>;
