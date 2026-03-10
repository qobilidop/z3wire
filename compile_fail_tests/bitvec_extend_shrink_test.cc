#include "z3wire/bitvec.h"

// extend<4, 8, false> should fail: target width < source width.
void trigger() {
    z3::context ctx;
    z3w::Ubv<8> val(ctx, "v");
    auto x = z3w::internal::extend<4, 8, false>(val);
}
