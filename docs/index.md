# Z3Wire

A type-safe C++20 template library for [Z3](https://github.com/Z3Prover/z3)
that enforces compile-time bit-width consistency and bit-growth arithmetic for
hardware modeling and formal verification.

## Why Z3Wire?

Z3's C++ API represents all expressions as `z3::expr`, with no compile-time
distinction between Booleans and bit-vectors, no width tracking, and no
signedness information. This means:

- Adding a 32-bit vector to an 8-bit vector **compiles silently** but crashes at
  runtime with `Z3_SORT_ERROR`.
- Comparing bit-vectors requires choosing the right function (`z3::ult` vs
  `z3::slt`), but nothing prevents calling the wrong one.
- Arithmetic silently discards overflow — adding two 8-bit values gives an 8-bit
  result, losing the carry bit.

Z3Wire fixes both problems:

- **Compile-time type safety** catches width and signedness mismatches as
  compiler errors.
- **Bit-growth arithmetic** widens results automatically, making every
  truncation an explicit, reviewable decision.

## Features

- **Compile-time type safety** — bit-width and signedness mismatches become
  compiler errors, not runtime Z3 sort errors.
- **Bit-growth arithmetic** — `+` and `-` automatically widen the result to
  prevent silent overflow.
- **Three-tier casting** — `cast` (raw hardware), `safe_cast` (compile-time
  lossless), `checked_cast` (symbolic verification).
- **Three-tier shifting** — `<<`/`>>` (hardware), `checked_shl`/`checked_shr`
  (detect lost bits), `lossless_shl` (auto-widen result).
- **Zero overhead** — each wrapper holds only a `z3::expr`.

## Quick Example

```cpp
#include <z3++.h>
#include "z3wire/bitvec.h"

z3::context ctx;
z3::solver solver(ctx);

z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> b(ctx, "b");

auto sum = a + b;  // z3w::Ubv<9>, no overflow possible

// Model hardware truncation explicitly
auto reg = z3w::cast<z3w::Ubv<8>>(sum);

// Verify the cast is safe
auto [result, overflowed] = z3w::checked_cast<z3w::Ubv<8>>(sum);
solver.add(!overflowed.raw());
```

## Target Audience

Hardware designers, verification engineers, and security researchers who use Z3
for formal verification of digital circuits and need type safety guarantees that
raw Z3 does not provide.
