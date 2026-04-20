<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/z3wire_logo_white.svg" width="128">
    <source media="(prefers-color-scheme: light)" srcset="docs/assets/z3wire_logo_black.svg" width="128">
    <img alt="Z3Wire" src="docs/assets/z3wire_logo_black.svg" width="128">
  </picture>
</p>
<p align="center">
  <a href="https://github.com/qobilidop/z3wire/actions/workflows/devcontainer.yml"><img src="https://github.com/qobilidop/z3wire/actions/workflows/devcontainer.yml/badge.svg" alt="Dev Container"></a>
  <a href="https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml"><img src="https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml/badge.svg" alt="Bazel"></a>
  <a href="https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml"><img src="https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml/badge.svg" alt="CMake"></a>
  <a href="https://github.com/qobilidop/z3wire/actions/workflows/lint.yml"><img src="https://github.com/qobilidop/z3wire/actions/workflows/lint.yml/badge.svg" alt="Lint"></a>
  <a href="https://qobilidop.github.io/z3wire/"><img src="https://github.com/qobilidop/z3wire/actions/workflows/docs.yml/badge.svg" alt="Docs"></a>
</p>

# Z3Wire

<!-- docs-start -->
<!-- --8<-- [start:intro] -->

Compile-time type-safe bit-vectors for Z3.

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
