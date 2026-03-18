# Bit Manipulation

This example demonstrates extract, concat, and shifts by modeling the unpacking
and repacking of a 16-bit instruction word.

```cpp title="examples/bit_manipulation.cc"
#include <iostream>

#include <z3++.h>

#include "z3wire/sym_bit_vec.h"

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  // A 16-bit instruction: [15:12] opcode | [11:8] dest | [7:0] immediate
  z3w::SymUInt<16> instruction(ctx, "instruction");

  // Unpack fields.
  auto opcode = z3w::extract<15, 12>(instruction);   // SymUInt<4>
  auto dest = z3w::extract<11, 8>(instruction);      // SymUInt<4>
  auto immediate = z3w::extract<7, 0>(instruction);  // SymUInt<8>

  // Repack.
  auto repacked = z3w::concat(opcode, dest, immediate);  // SymUInt<16>

  // Verify round-trip: repacked == original.
  solver.add((repacked != instruction).expr());

  if (solver.check() == z3::unsat) {
    std::cout << "Verified: extract/concat round-trip is lossless.\n";
  } else {
    std::cout << "Bug found!\n";
  }

  // Demonstrate left shift: shift an 8-bit value left by 3 without
  // losing any bits.
  z3w::SymUInt<8> value(ctx, "value");
  auto shifted = z3w::shl<3>(value);  // SymUInt<11>

  // Verify we can recover the original by shifting back.
  auto recovered = z3w::extract<10, 3>(shifted);

  z3::solver s2(ctx);
  s2.add((recovered != value).expr());

  if (s2.check() == z3::unsat) {
    std::cout << "Verified: shl preserves all bits.\n";
  } else {
    std::cout << "Bug found!\n";
  }

  return 0;
}
```

## What this demonstrates

- **`extract`:** Slicing a word into fields with compile-time bounds checking.
- **`concat`:** Variadic concatenation to repack fields.
- **`shl`:** Left shift that widens the result type automatically, preserving
    all bits.
- **Round-trip verification:** Proving that unpack/repack preserves all
    information.

## Running

```sh
./dev.sh bazel run //examples:bit_manipulation
```

Output:

```
Verified: extract/concat round-trip is lossless.
Verified: shl preserves all bits.
```
