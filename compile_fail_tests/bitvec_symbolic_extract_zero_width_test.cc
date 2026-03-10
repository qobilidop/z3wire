#include "z3wire/bitvec.h"

// Symbolic extract<0> should fail: TargetWidth must be > 0.
void trigger() {
    z3::context ctx;
    z3w::Ubv<8> val(ctx, "v");
    z3w::Ubv<3> idx(ctx, "i");
    auto x = z3w::extract<0>(val, idx);
}
