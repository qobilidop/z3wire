# Z3Wire

[![Devcontainer](https://github.com/qobilidop/z3wire/actions/workflows/devcontainer.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/devcontainer.yml)
[![Bazel](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml)
[![CMake](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml)
[![Checks](https://github.com/qobilidop/z3wire/actions/workflows/checks.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/checks.yml)
[![codecov](https://codecov.io/gh/qobilidop/z3wire/graph/badge.svg)](https://codecov.io/gh/qobilidop/z3wire)
[![Docs](https://github.com/qobilidop/z3wire/actions/workflows/docs.yml/badge.svg)](https://qobilidop.github.io/z3wire/)

<!-- docs-start -->

Type-safe Z3 bit-vectors for hardware verification. C++20 and above.

## Why Z3Wire?

Using Z3 bit-vectors directly for hardware verification is error-prone:

- **Width mismatches are silent.** Adding a 32-bit vector to an 8-bit vector
    compiles fine but crashes at runtime with `Z3_SORT_ERROR`.
- **Signedness is unchecked.** Comparing bit-vectors requires choosing the right
    function (`z3::ult` vs `z3::slt`), but nothing prevents calling the wrong one.
- **Overflow requires vigilance.** Arithmetic silently wraps by default. Z3
    provides overflow predicates (`bvadd_no_overflow`, etc.), but they are opt-in
    and easy to forget — a missed check means a proof may pass because the formula
    lost information, not because the design is correct.

Z3Wire solves these by bringing hardware semantics into the type system:

- **Compile-time type safety** — width and signedness mismatches become compile-time
    errors, not runtime surprises.
- **Bit-growth arithmetic** — results widen automatically, making every
    truncation an explicit, reviewable decision.

## What's in the name?

The name reflects the scope: hardware is built from *wires*. Every signal in a
digital circuit is either a single bit or a bundle of bits with a known width.
Z3Wire wraps Z3 with type-safe Booleans and fixed-width bit-vectors, covering
the complete set of [combinational logic primitives](https://qobilidop.github.io/z3wire/design/overview/#combinational-logic-primitives) that operate on these wires.

## Features

- **Three-tier casting** — `safe_cast` (compile-time lossless),
    `checked_cast` (symbolic verification), `unsafe_cast` (raw hardware).
    `as_signed`/`as_unsigned` for same-width reinterpretation.
- **Shifting** — `shl` (auto-widening left shift), `shr` (arithmetic right
    shift).
- **Zero overhead** — each wrapper holds only a `z3::expr`.

## Quick example

```cpp
#include <z3++.h>
#include "z3wire/sym_bit_vec.h"

z3::context ctx;
z3::solver solver(ctx);

// Verify that a carry flag correctly detects 8-bit addition overflow.
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
auto sum = a + b;  // z3w::SymUInt<9>
auto carry = z3w::as_bool(z3w::extract<8, 8>(sum));  // bit 8 = carry
auto [truncated, value_preserved] = z3w::checked_cast<z3w::SymUInt<8>>(sum);

// Ask Z3: is there any case where carry == value_preserved?
// (carry=true means overflow, value_preserved=false means overflow, so they
// should always be opposite.)
solver.add((carry == value_preserved).expr());
assert(solver.check() == z3::unsat);  // No — carry is always correct.
```

See the [safe adder example](https://qobilidop.github.io/z3wire/examples/safe-adder/) for a full walkthrough.

## Getting started

Requires [Docker](https://www.docker.com/). Clone the repo and run:

```sh
./dev.sh bazel run //examples:safe_adder
```

## Documentation

Visit [qobilidop.github.io/z3wire](https://qobilidop.github.io/z3wire/) for the
full documentation, including getting started guide, user guide, and examples.

## License

See [LICENSE](https://github.com/qobilidop/z3wire/blob/main/LICENSE).

<!-- docs-end -->
