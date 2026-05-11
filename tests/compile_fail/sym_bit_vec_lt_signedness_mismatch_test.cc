#include "z3wire/sym_bit_vec.h"

// SymUInt<8> < SymSInt<8> must fail to compile: signedness differs.
void trigger() {
    z3::context ctx;
    z3w::SymUInt<8> a(ctx, "a");
    z3w::SymSInt<8> b(ctx, "b");
    auto x = (a < b);
}
