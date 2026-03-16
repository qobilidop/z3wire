#include "z3wire/sym_int.h"

// extract<0, 3> should fail: High (0) must be >= Low (3).
void trigger() {
    z3::context ctx;
    z3w::SymUInt<8> val(ctx, "v");
    auto x = z3w::extract<0, 3>(val);
}
