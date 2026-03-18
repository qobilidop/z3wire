# Z3Wire

[![Devcontainer](https://github.com/qobilidop/z3wire/actions/workflows/devcontainer.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/devcontainer.yml)
[![Bazel](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml)
[![CMake](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml)
[![Lint](https://github.com/qobilidop/z3wire/actions/workflows/checks.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/checks.yml)
[![codecov](https://codecov.io/gh/qobilidop/z3wire/graph/badge.svg)](https://codecov.io/gh/qobilidop/z3wire)
[![Docs](https://github.com/qobilidop/z3wire/actions/workflows/docs.yml/badge.svg)](https://qobilidop.github.io/z3wire/)

<!-- docs-start -->

Type-safe Z3 bit-vectors for hardware verification. C++20 and above.

## Why Z3Wire?

Using Z3 bit-vectors directly for hardware verification is error-prone:

- **Width mismatches are silent.** Adding a 32-bit vector to an 8-bit vector
    compiles fine but crashes at runtime with `Z3_SORT_ERROR`.
- **Signedness is unchecked.** Comparing bit-vectors requires choosing the right
    function (`z3::ult` vs `z3::slt`), but nothing prevents calling the wrong
    one.
- **Overflow requires vigilance.** Arithmetic silently wraps by default. Z3
    provides overflow predicates (`bvadd_no_overflow`, etc.), but they are
    opt-in and easy to forget — a missed check means a proof may pass because
    the formula lost information, not because the design is correct.

Z3Wire solves these by bringing hardware semantics into the type system:

- **Compile-time type safety** — width and signedness mismatches become
    compile-time errors, not runtime surprises.
- **Bit-growth arithmetic** — results widen automatically, making every
    truncation an explicit, reviewable decision.

## What's in the name?

The name reflects the scope: hardware is built from *wires*. Every signal in a
digital circuit is either a single bit or a bundle of bits with a known width.
Z3Wire wraps Z3 with type-safe Booleans and fixed-width bit-vectors, covering
the complete set of
[combinational logic primitives](https://qobilidop.github.io/z3wire/design/overview/#combinational-logic-primitives)
that operate on these wires.

## Quick example

The classic binary search midpoint formula `(a + b) >> 1` has an overflow bug
that lurked in Java's `Arrays.binarySearch` for 9 years before
[Joshua Bloch discovered it in 2006](https://research.google/blog/extra-extra-read-all-about-it-nearly-all-binary-searches-and-mergesorts-are-broken/).
Can we prove the well-known bit-hack fix
[`(a & b) + ((a ^ b) >> 1)`](https://devblogs.microsoft.com/oldnewthing/20220207-00/?p=106223)
is correct?

```cpp
#include <z3++.h>
#include "z3wire/bit_vec.h"
#include "z3wire/sym_bit_vec.h"

z3::context ctx;
z3::solver solver(ctx);

z3w::SymUInt<32> a(ctx, "a");
z3w::SymUInt<32> b(ctx, "b");
auto one = z3w::UInt<32>::Literal<1>();

// Result types are derived at compile time. Written out explicitly for clarity.

// Buggy: ((uint32_t) (a + b)) >> 1
// Truncating sum wraps, then shift gives wrong answer.
z3w::SymUInt<32> buggy =
    z3w::shr(z3w::unsafe_cast<z3w::SymUInt<32>>(a + b), one);

// Bit-hack fix: (uint32_t) ((a & b) + ((a ^ b) >> 1))
// Magical! We shall prove it's correct.
z3w::SymUInt<32> hack =
    z3w::unsafe_cast<z3w::SymUInt<32>>((a & b) + z3w::shr(a ^ b, one));

// Z3Wire: (a + b) >> 1
// Expresses the intended correct semantics naturally.
z3w::SymUInt<33> correct = z3w::shr(a + b, one);

// Prove the buggy version can produce wrong results.
solver.push();
solver.add((buggy != correct).expr());
assert(solver.check() == z3::sat);  // Yes — overflow exists.
solver.pop();

// Prove the bit-hack always matches the correct result.
solver.push();
solver.add((hack != correct).expr());
assert(solver.check() == z3::unsat);  // Proven correct for all inputs.
solver.pop();
```

Check out
[`examples/midpoint_overflow.cc`](https://github.com/qobilidop/z3wire/blob/main/examples/midpoint_overflow.cc)
for the full example.

## Getting started

Visit [qobilidop.github.io/z3wire](https://qobilidop.github.io/z3wire/) for the
full documentation.

## License

[Z3 uses MIT License](https://github.com/Z3Prover/z3/blob/master/LICENSE.txt)
and [we followed](https://github.com/qobilidop/z3wire/blob/main/LICENSE).

<!-- docs-end -->
