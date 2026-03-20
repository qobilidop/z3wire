#include "z3wire/sym_bit_vec.h"

// safe_cast SymUInt<8> -> SymUInt<4> should fail: target too narrow.
void trigger() {
    z3::context ctx;
    z3w::SymUInt<8> val(ctx, "v");
    auto x = z3w::safe_cast<z3w::SymUInt<4>>(val);
}
