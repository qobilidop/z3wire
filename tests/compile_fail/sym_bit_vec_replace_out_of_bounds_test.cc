#include "z3wire/sym_bit_vec.h"

// replace<LO=5>(SymUInt<8> src, SymUInt<8> field) should fail:
// LO + WF (13) > WS (8).
void f(const z3w::SymUInt<8>& src, const z3w::SymUInt<8>& field) {
    auto x = z3w::replace<5>(src, field);
}
