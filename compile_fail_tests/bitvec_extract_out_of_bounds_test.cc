#include "z3wire/bitvec.h"

// extract<8, 0> on Ubv<8> should fail: High (8) must be < width (8).
void trigger() {
    z3::context ctx;
    z3w::Ubv<8> val(ctx, "v");
    auto x = z3w::extract<8, 0>(val);
}
