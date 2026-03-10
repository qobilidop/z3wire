#include "z3wire/int.h"
// Explicitly instantiate the int64_t overload template with default args.
// For UInt<8>, IsSigned=false, so enable_if_t<false> causes SFINAE failure.
void f() {
  z3w::UInt<8>::checked<>(int64_t{42});
}
