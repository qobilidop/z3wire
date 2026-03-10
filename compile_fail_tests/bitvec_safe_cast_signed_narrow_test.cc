#include "z3wire/bitvec.h"

// safe_cast Sbv<8> -> Sbv<4> should fail: target too narrow.
void trigger() {
    z3::context ctx;
    z3w::Sbv<8> val(ctx, "v");
    auto x = z3w::safe_cast<z3w::Sbv<4>>(val);
}
