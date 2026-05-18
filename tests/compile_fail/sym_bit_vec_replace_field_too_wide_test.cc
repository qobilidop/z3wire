#include "z3wire/sym_bit_vec.h"

// replace(SymUInt<8> src, SymUInt<16> field, size_t) should fail:
// WF (16) > WS (8).
void f(const z3w::SymUInt<8>& src, const z3w::SymUInt<16>& field) {
    auto x = z3w::replace(src, field, size_t{0});
}
