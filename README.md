# Z3Wire

[![Dev Container](https://github.com/qobilidop/z3wire/actions/workflows/devcontainer.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/devcontainer.yml)
[![Bazel](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml)
[![CMake](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml)
[![Lint](https://github.com/qobilidop/z3wire/actions/workflows/lint.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/lint.yml)
[![Docs](https://github.com/qobilidop/z3wire/actions/workflows/docs.yml/badge.svg)](https://qobilidop.github.io/z3wire/)

<!-- docs-start -->
<!-- --8<-- [start:intro] -->

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

## Z3 vs Z3Wire

Multiply an unsigned 8-bit value by a signed 8-bit value. In raw Z3, you must
manually extend each operand to the correct width with the correct signedness -
and nothing stops you from getting it wrong:

```cpp
// Z3
z3::context ctx;
z3::expr a = ctx.bv_const("a", 8);           // unsigned (by convention)
z3::expr b = ctx.bv_const("b", 8);           // signed (by convention)
z3::expr a_wide = z3::zext(a, 8);            // zero-extend to 16 bits
z3::expr b_wide = z3::sext(b, 8);            // sign-extend to 16 bits
z3::expr product = a_wide * b_wide;          // signed 16-bit (by convention)
// Mix up sext/zext? Wrong answer, no error.
```

Z3Wire makes it correct by construction:

```cpp
// Z3Wire
z3::context ctx;
z3w::SymUInt<8> a(ctx, "a");
z3w::SymSInt<8> b(ctx, "b");
auto product = a * b;                        // SymSInt<16>, guaranteed correct
```

Width, signedness, and overflow safety are all enforced at compile time.

## Get started

Visit [qobilidop.github.io/z3wire](https://qobilidop.github.io/z3wire/) for the
full documentation.

<!-- --8<-- [end:intro] -->
<!-- docs-end -->
