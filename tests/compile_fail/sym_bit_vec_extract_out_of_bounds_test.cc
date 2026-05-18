#include "z3wire/sym_bit_vec.h"

// extract<W=2, Lo=7> on SymUInt<8> should fail: Lo + W (9) > source width (8).
void f(const z3w::SymUInt<8>& val) {
    auto x = z3w::extract<2, 7>(val);
}
