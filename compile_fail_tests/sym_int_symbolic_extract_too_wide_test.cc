#include "z3wire/sym_int.h"

// Symbolic extract<9> on SymUInt<8> should fail: TargetWidth must be <= width.
void trigger() {
    z3::context ctx;
    z3w::SymUInt<8> val(ctx, "v");
    z3w::SymUInt<3> idx(ctx, "i");
    auto x = z3w::extract<9>(val, idx);
}
