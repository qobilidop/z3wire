#include "z3wire/sym_int.h"

// SymUInt<8> max is 255; Literal<256> should fail.
void trigger() {
    z3::context ctx;
    auto x = z3w::SymUInt<8>::Literal<256>(ctx);
}
