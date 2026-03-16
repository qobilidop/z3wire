#include "z3wire/sym_bit_vec.h"

// SymBitVec<0, false> should fail: "Width must be at least 1."
template class z3w::SymBitVec<0, false>;
