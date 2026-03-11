# Bit Manipulation

This example demonstrates extract, concat, and lossless shifts by modeling the
unpacking and repacking of a 16-bit instruction word.

```cpp title="examples/bit_manipulation.cc"
#include <iostream>

#include <z3++.h>

#include "z3wire/bitvec.h"

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  // A 16-bit instruction: [15:12] opcode | [11:8] dest | [7:0] immediate
  z3w::Ubv<16> instruction(ctx, "instruction");

  // Unpack fields.
  auto opcode = z3w::extract<15, 12>(instruction);   // Ubv<4>
  auto dest = z3w::extract<11, 8>(instruction);      // Ubv<4>
  auto immediate = z3w::extract<7, 0>(instruction);  // Ubv<8>

  // Repack.
  auto repacked = z3w::concat(opcode, dest, immediate);  // Ubv<16>

  // Verify round-trip: repacked == original.
  solver.add((repacked != instruction).raw());

  if (solver.check() == z3::unsat) {
    std::cout << "Verified: extract/concat round-trip is lossless.\n";
  } else {
    std::cout << "Bug found!\n";
  }

  // Demonstrate lossless shift: shift an 8-bit value left by 3 without
  // losing any bits.
  z3w::Ubv<8> value(ctx, "value");
  auto shifted = z3w::lossless_shl<3>(value);  // Ubv<11>

  // Verify we can recover the original by shifting back.
  auto recovered = z3w::extract<10, 3>(shifted);

  z3::solver s2(ctx);
  s2.add((recovered != value).raw());

  if (s2.check() == z3::unsat) {
    std::cout << "Verified: lossless_shl preserves all bits.\n";
  } else {
    std::cout << "Bug found!\n";
  }

  return 0;
}
```

## What this demonstrates

- **`extract`:** Slicing a word into fields with compile-time bounds checking.
- **`concat`:** Variadic concatenation to repack fields.
- **`lossless_shl`:** Shifting without losing bits (result type widens
    automatically).
- **Round-trip verification:** Proving that unpack/repack preserves all
    information.

## Running

```sh
./dev.sh bazel run //examples:bit_manipulation
```

Output:

```
Verified: extract/concat round-trip is lossless.
Verified: lossless_shl preserves all bits.
```
