# Comparison Semantics

Why Z3Wire comparison operators are strict on width and signedness, and why
mathematical comparisons are surfaced as the `z3w::math_*` free-function family.

## The decision

The six symbolic comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`)
require both operands to have the same width and signedness. A mismatch is a
compile error.

For cross-type comparison, use the parallel free-function family `math_eq`,
`math_ne`, `math_lt`, `math_le`, `math_gt`, `math_ge`. These widen operands
to a common type internally and answer the question: *do these operands
represent the same mathematical value (or stand in the correct ordering)?*

```cpp
SymUInt<8> a(ctx, "a");
SymUInt<8> b(ctx, "b");
SymBool eq = (a == b);              // OK: same type.

auto sum = a + b;                   // SymUInt<9>
SymBool overflow = math_gt(sum, UInt<8>::Literal<255>());  // mathematical

SymSInt<8> s(ctx, "s");
SymBool meaning = math_eq(a, s);    // explicit mathematical equality
// (a == s) would be a compile error pointing the user at math_eq or a cast.
```

## Why strict by default

Z3Wire's other binary operators uphold an "explicit over implicit" principle:
bitwise operators require matching width and signedness, and arithmetic uses
bit-growth (the result type encodes the widening; the operands themselves are
not silently extended). Comparison is the lone exception worth re-examining.

Two concerns argued against the original "mathematical default":

1.  **Users from C++/Verilog backgrounds have no positive intuition for
    mixed-signedness comparison.** Both languages have well-known footguns
    there. Z3Wire's heterogeneous behavior was mathematically *correct* in
    all cases, but silent — a user reading `a == b` could not tell from the
    call site whether the operands matched in type, or which question the
    comparison was answering.

2.  **For `==` there is a real semantic choice that the silent default
    hides.** `SymUInt<8>(255) == SymSInt<8>(-1)` is `false` mathematically
    but `true` if compared as bit patterns. Strict matching forces the user
    to choose: `math_eq(a, b)` for the mathematical question; cast then `==`
    for the bit-pattern question.

The strict design aligns comparison with the rest of the library and matches
the dominant pattern across modern strongly-typed languages (Rust, Go,
Haskell, Zig). The `math_*` family preserves the heterogeneous mathematical
operation as an explicit, named call — the same pattern C++20 chose for
`std::cmp_equal`, `std::cmp_less`, and their siblings.

## Widening rule (used by `math_*`)

Both operands are extended to a **common width** that preserves every value
from both sides before the underlying Z3 comparison is performed:

| Operand signedness | Common width    | Extension method                             |
| ------------------ | --------------- | -------------------------------------------- |
| Both unsigned      | max(W1, W2)     | Narrower side zero-extended                  |
| Both signed        | max(W1, W2)     | Narrower side sign-extended                  |
| Mixed              | max(W1, W2) + 1 | Unsigned zero-extended, signed sign-extended |

For ordered `math_*` calls, the signedness of the underlying comparison is
determined by the operands: both unsigned uses `ult`/`ule`/`ugt`/`uge`;
either signed uses `slt`/`sle`/`sgt`/`sge` after widening. The mixed-width
"+1 bit" rule guarantees the unsigned operand's full range fits in the
signed domain.

## Contrast with other operators

| Operator class | Width policy             | Why                                                            |
| -------------- | ------------------------ | -------------------------------------------------------------- |
| Comparison     | Strict (`==`, `<`, …)    | Aligns with the rest of the library; no silent type-crossing.  |
| Comparison     | Heterogeneous (`math_*`) | Explicit free function names the mathematical question.        |
| Arithmetic     | Bit-growth               | Result width encodes overflow capacity.                        |
| Bitwise        | Strict match             | Bit positions must align; widening changes semantics.          |

## Reference

- C++20 `<utility>` `std::cmp_equal` family — the direct precedent for
  surfacing mathematical comparison as named free functions.
- Rust `PartialEq<Rhs = Self>` / `PartialOrd<Rhs = Self>` — strict-typed
  comparison as the default in a modern type system.
