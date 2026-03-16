#include "z3wire/sym_int.h"

// safe_cast SymUInt<8> -> SymSInt<8> should fail: needs target width > source.
void trigger() {
    z3::context ctx;
    z3w::SymUInt<8> val(ctx, "v");
    auto x = z3w::safe_cast<z3w::SymSInt<8>>(val);
}
