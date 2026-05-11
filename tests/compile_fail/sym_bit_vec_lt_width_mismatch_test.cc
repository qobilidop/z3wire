#include "z3wire/sym_bit_vec.h"

// SymUInt<8> < SymUInt<16> must fail to compile: widths differ.
void trigger() {
    z3::context ctx;
    z3w::SymUInt<8> a(ctx, "a");
    z3w::SymUInt<16> b(ctx, "b");
    auto x = (a < b);
}
