#include "z3wire/sym_bit_vec.h"

// extract<8, 0> on SymUInt<8> should fail: High (8) must be < width (8).
void trigger() {
    z3::context ctx;
    z3w::SymUInt<8> val(ctx, "v");
    auto x = z3w::extract<8, 0>(val);
}
