#include "z3wire/bit_vec.h"

// BitVec<65, true> should fail: signed bit-vectors wider than 64 bits.
template class z3w::BitVec<65, true>;
