#include "z3wire/bitfield.h"

// Field widths (3 + 3 = 6) do not sum to buffer width (8).
void trigger() {
  z3::context ctx;
  z3w::Ubv<8> buf(ctx, "buf");
  z3w::Ubv<3> a(ctx, "a");
  z3w::Ubv<3> b(ctx, "b");
  auto eq = z3w::bitfield_eq(buf, a, b);
}
