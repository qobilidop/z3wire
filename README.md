# Z3Wire

[![Dev Container](https://github.com/qobilidop/z3wire/actions/workflows/devcontainer.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/devcontainer.yml)
[![Bazel](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml)
[![CMake](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml)
[![Lint](https://github.com/qobilidop/z3wire/actions/workflows/lint.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/lint.yml)
[![codecov](https://codecov.io/gh/qobilidop/z3wire/graph/badge.svg)](https://codecov.io/gh/qobilidop/z3wire)
[![Docs](https://github.com/qobilidop/z3wire/actions/workflows/docs.yml/badge.svg)](https://qobilidop.github.io/z3wire/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/qobilidop/z3wire/blob/main/LICENSE)

<!-- docs-start -->

Type-safe Z3 bit-vectors for hardware verification. C++20 and above.

Named for the wires in digital circuits: single bits and fixed-width bundles,
now type-safe.

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

- **Compile-time type safety** — width and signedness mismatches become
  compile-time errors, not runtime surprises.
- **Bit-growth arithmetic** — results widen automatically, making every
  truncation an explicit, reviewable decision.

## Quick example

The classic binary search midpoint formula `(a + b) >> 1` has an overflow bug
that lurked in Java's `Arrays.binarySearch` for 9 years before
[Joshua Bloch discovered it in 2006](https://research.google/blog/extra-extra-read-all-about-it-nearly-all-binary-searches-and-mergesorts-are-broken/).
Can we prove the well-known bit-hack fix
[`(a & b) + ((a ^ b) >> 1)`](https://devblogs.microsoft.com/oldnewthing/20220207-00/?p=106223)
is correct? Here's how it looks with Z3Wire:

```cpp
z3w::SymUInt<32> a(ctx, "a");
z3w::SymUInt<32> b(ctx, "b");
auto one = z3w::UInt<32>::Literal<1>();

// Original `(a + b) >> 1` in hardware: 32-bit sum wraps, giving wrong answer.
z3w::SymUInt<32> buggy =
    z3w::shr(z3w::unsafe_cast<z3w::SymUInt<32>>(a + b), one);

// Bit-hack fix `(a & b) + ((a ^ b) >> 1)`: avoids overflow via bitwise tricks.
z3w::SymUInt<32> hack =
    z3w::unsafe_cast<z3w::SymUInt<32>>((a & b) + z3w::shr(a ^ b, one));

// Correct semantics: 33-bit sum can't overflow, result is exact.
z3w::SymUInt<33> correct = z3w::shr(a + b, one);
```

The full example uses Z3 to prove that `buggy` can produce wrong results and
that `hack` always matches `correct` for all 32-bit inputs. The key insight:
`a + b` returns `SymUInt<33>`, so overflow is impossible, and truncation
requires an explicit `unsafe_cast`.

See
[`examples/midpoint_overflow.cc`](https://github.com/qobilidop/z3wire/blob/main/examples/midpoint_overflow.cc)
for the full example.

## Getting started

To run the quick example yourself, make sure you have
[Docker and Dev Container CLI](https://qobilidop.github.io/z3wire/dev/guide/#prerequisites),
then:

```sh
git clone https://github.com/qobilidop/z3wire.git
cd z3wire
./dev.sh bazel run //examples:midpoint_overflow
```

Visit [qobilidop.github.io/z3wire](https://qobilidop.github.io/z3wire/) for the
full documentation.

<!-- docs-end -->
