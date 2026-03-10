#include "z3wire/bitvec.h"

// safe_cast from signed to unsigned is always forbidden.
void trigger() {
    z3::context ctx;
    z3w::Sbv<8> val(ctx, "v");
    auto x = z3w::safe_cast<z3w::Ubv<8>>(val);
}
