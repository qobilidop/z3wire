#include "z3wire/sym_bit_vec.h"

// extract<W=0, Lo=0> should fail: W must be > 0.
void f(const z3w::SymUInt<8>& val) {
    auto x = z3w::extract<0, 0>(val);
}
