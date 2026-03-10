#include "z3wire/bitvec.h"

// safe_cast Ubv<8> -> Sbv<8> should fail: needs target width > source.
void trigger() {
    z3::context ctx;
    z3w::Ubv<8> val(ctx, "v");
    auto x = z3w::safe_cast<z3w::Sbv<8>>(val);
}
