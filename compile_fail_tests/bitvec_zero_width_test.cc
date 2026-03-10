#include "z3wire/bitvec.h"

// BitVec<0, false> should fail: "Bit-vector width must be at least 1."
template class z3w::BitVec<0, false>;
