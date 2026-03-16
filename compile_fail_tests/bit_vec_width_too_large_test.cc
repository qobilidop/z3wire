#include "z3wire/bit_vec.h"

// BitVec<65, false> should fail: "Bit-width must be between 1 and 64."
template class z3w::BitVec<65, false>;
