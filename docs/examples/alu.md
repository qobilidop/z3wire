# ALU Verification

This example models a simple 4-operation ALU and verifies its properties.

The ALU takes two 8-bit inputs and a 2-bit opcode, producing an 8-bit result.
We verify that the AND operation never exceeds its inputs, and find an example
of ADD overflow.

```cpp title="examples/alu.cc"
#include <iostream>

#include <z3++.h>

#include "z3wire/bitvec.h"

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  z3w::Ubv<8> a(ctx, "a");
  z3w::Ubv<8> b(ctx, "b");
  z3w::Ubv<2> opcode(ctx, "opcode");

  // ALU operations (all truncated to 8-bit hardware width).
  auto add_result = z3w::cast<z3w::Ubv<8>>(a + b);
  auto sub_result = z3w::cast<z3w::Ubv<8>>(a - b);
  auto and_result = a & b;
  auto or_result = a | b;

  // Mux the result based on opcode.
  auto op0 = z3w::Ubv<2>::Literal<0>(ctx);
  auto op1 = z3w::Ubv<2>::Literal<1>(ctx);
  auto op2 = z3w::Ubv<2>::Literal<2>(ctx);

  auto result =
      z3w::ite(opcode == op0, add_result,
               z3w::ite(opcode == op1, sub_result,
                        z3w::ite(opcode == op2, and_result, or_result)));

  // Property: for AND operation, result is always <= both inputs.
  solver.push();
  solver.add(opcode.raw() == op2.raw());
  solver.add((result > a).raw() || (result > b).raw());

  if (solver.check() == z3::unsat) {
    std::cout << "Verified: AND result is always <= both inputs.\n";
  } else {
    std::cout << "Bug found!\n";
  }
  solver.pop();

  // Property: for ADD, find inputs where overflow occurs.
  auto sum9 = a + b;
  auto [_, overflowed] = z3w::checked_cast<z3w::Ubv<8>>(sum9);

  solver.push();
  solver.add(overflowed.raw());

  if (solver.check() == z3::sat) {
    auto model = solver.get_model();
    std::cout << "ADD overflow example: a=" << model.eval(a.raw())
              << ", b=" << model.eval(b.raw()) << "\n";
  }
  solver.pop();

  return 0;
}
```

## What this demonstrates

- **`ite`:** Building a mux (multiplexer) from symbolic conditions.
- **`Literal`:** Compile-time range-checked constants for opcodes.
- **`checked_cast`:** Detecting overflow symbolically.
- **`push`/`pop`:** Using Z3's solver stack to check multiple properties.
- **Verification vs. finding examples:** `unsat` proves a property;
  `sat` finds a concrete witness.

## Running

```sh
./dev.sh bazel run //examples:alu
```

Output:

```
Verified: AND result is always <= both inputs.
ADD overflow example: a=1, b=255
```
