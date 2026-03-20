#include "z3wire/sym_bit_vec.h"

// safe_cast from signed to unsigned is always forbidden.
void trigger() {
    z3::context ctx;
    z3w::SymSInt<8> val(ctx, "v");
    auto x = z3w::safe_cast<z3w::SymUInt<8>>(val);
}
