#include "z3wire/sym_int.h"

// SymInt<0, false> should fail: "Width must be at least 1."
template class z3w::SymInt<0, false>;
