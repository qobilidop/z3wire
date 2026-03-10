#include "z3wire/bitvec.h"

// safe_cast Ubv<8> -> Ubv<4> should fail: target too narrow.
void trigger() {
    z3::context ctx;
    z3w::Ubv<8> val(ctx, "v");
    auto x = z3w::safe_cast<z3w::Ubv<4>>(val);
}
