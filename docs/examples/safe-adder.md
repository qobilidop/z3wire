# Safe Adder

This example verifies that a carry flag correctly detects 8-bit addition
overflow.

A common hardware pattern: add two 8-bit values, produce an 8-bit result and a
carry flag. Z3Wire's bit-growth arithmetic lets us verify this without manually
managing widths.

```cpp title="examples/safe_adder.cc"
#include <iostream>

#include <z3++.h>

#include "z3wire/sym_bit_vec.h"

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymUInt<8> b(ctx, "b");
  auto sum = a + b;                                    // z3w::SymUInt<9>
  auto carry = z3w::to_bool(z3w::extract<8, 8>(sum));  // bit 8 = carry
  auto [truncated, overflowed] = z3w::checked_cast<z3w::SymUInt<8>>(sum);

  // Ask Z3: is there any case where carry != overflowed?
  solver.add((carry != overflowed).raw());

  if (solver.check() == z3::unsat) {
    std::cout << "Verified: carry flag matches overflow detection.\n";
  } else {
    std::cout << "Bug found! Counter-example:\n";
    auto model = solver.get_model();
    std::cout << "  a = " << model.eval(a.raw()) << "\n";
    std::cout << "  b = " << model.eval(b.raw()) << "\n";
  }

  return 0;
}
```

## What this demonstrates

- **Bit-growth arithmetic:** `a + b` produces a 9-bit result, so no information
    is lost.
- **`extract`:** Pulling out the carry bit.
- **`checked_cast`:** Getting a symbolic overflow flag.
- **Verification pattern:** Assert a property and check for `unsat` to prove it
    holds for all possible inputs.

## Running

```sh
./dev.sh bazel run //examples:safe_adder
```

Output:

```
Verified: carry flag matches overflow detection.
```
