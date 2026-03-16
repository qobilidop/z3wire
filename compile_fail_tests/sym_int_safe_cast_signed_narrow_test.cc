#include "z3wire/sym_int.h"

// safe_cast SymSInt<8> -> SymSInt<4> should fail: target too narrow.
void trigger() {
    z3::context ctx;
    z3w::SymSInt<8> val(ctx, "v");
    auto x = z3w::safe_cast<z3w::SymSInt<4>>(val);
}
