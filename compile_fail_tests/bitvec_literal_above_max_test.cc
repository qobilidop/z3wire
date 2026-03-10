#include "z3wire/bitvec.h"

// Ubv<8> max is 255; Literal<256> should fail.
void trigger() {
    z3::context ctx;
    auto x = z3w::Ubv<8>::Literal<256>(ctx);
}
