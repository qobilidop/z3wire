# Shift API Redesign

Simplify the shift API by removing redundant functions and using named
functions consistently.

## Motivation

The current shift API has three tiers (raw, checked, lossless) mirroring
the cast API. But unlike casts, checked and raw shifts are trivially
composable from the lossless shift and the cast API:

- `checked_shl(val, amt)` = `checked_cast(shl(val, amt))`
- `val << amt` = `unsafe_cast(shl(val, amt))`
- `checked_shr(val, amt)` has no compelling use case

Operator overloading (`<<`, `>>`) is also inconsistent with the rest of
the API, which uses named functions for casts and other operations.

This spec is the companion to [cast-rename.md](cast-rename.md), which
deferred shift operations to a separate spec.

## Current API

| Name                | Semantics                   | Returns                          |
| :------------------ | :-------------------------- | :------------------------------- |
| `val << amt`        | Raw left shift, same width  | `SymBitVec<W, S>`                |
| `val >> amt`        | Raw right shift, same width | `SymBitVec<W, S>`                |
| `checked_shl()`     | Left shift + loss flag      | `tuple<SymBitVec<W,S>, SymBool>` |
| `checked_shr()`     | Right shift + loss flag     | `tuple<SymBitVec<W,S>, SymBool>` |
| `lossless_shl<N>()` | Constant left shift, widens | `SymUInt<W+N>`                   |
| `lossless_shl()`    | Symbolic left shift, widens | `SymUInt<W+2^K-1>`               |

## New API

| Name               | Semantics                          | Returns            |
| :----------------- | :--------------------------------- | :----------------- |
| `shl<N>(val)`      | Constant left shift, widens        | `SymUInt<W+N>`     |
| `shl(val, amt)`    | Symbolic left shift, widens        | `SymUInt<W+2^K-1>` |
| `shr(val, amt)`    | Arithmetic right shift             | `SymBitVec<W, S>`  |
| `as_signed(val)`   | Same-width reinterpret to signed   | `SymSInt<W>`       |
| `as_unsigned(val)` | Same-width reinterpret to unsigned | `SymUInt<W>`       |

## Design decisions

### Named functions over operators

Shift operators (`<<`, `>>`) are dropped in favor of `shl`/`shr`. This
is consistent with the cast API (which uses named functions) and avoids
any ambiguity about result width.

### `shl` is always lossless

There is only one left shift primitive, and it always widens the result
to guarantee no bit loss. The `lossless_` prefix is dropped since there
is no other left shift to distinguish from. `shl` always returns
`SymUInt` regardless of input signedness, consistent with other raw bit
operations like `concat` and `extract`. Users who want same-width left
shift compose with `unsafe_cast` or `checked_cast`.

### `shr` is arithmetic

`shr` dispatches based on signedness: `z3::ashr` for signed types,
`z3::lshr` for unsigned types. The results are equivalent for unsigned
(sign bit is 0), but dispatching preserves the expected SMT operation
in generated formulas.

Signature: `shr(SymBitVec<W, S> val, SymBitVec<W, S> amt)` - both
operands must match in width and signedness (same as the current
`operator>>`).

Users who need logical right shift on signed values compose with
`as_unsigned`:

```cpp
shr(as_unsigned(signed_val), as_unsigned(amt))
```

### No `shr<N>` overload

Unlike `shl<N>`, a constant shift amount for right shift provides no
type-level benefit - the result is always width `W`. YAGNI.

### No checked shifts

- `checked_shl` is `checked_cast(shl(...))` - composable.
- `checked_shr` has no compelling use case.

### `as_signed` / `as_unsigned`

Same-width reinterpretation helpers that wrap `unsafe_cast` with automatic
type deduction. They belong to the cast API conceptually but are motivated
by the shift redesign, where they enable ergonomic composition patterns.

## Composability

| Pattern             | Composition                             |
| :------------------ | :-------------------------------------- |
| Raw left shift      | `unsafe_cast<SymUInt<W>>(shl<N>(val))`  |
| Checked left shift  | `checked_cast<SymUInt<W>>(shl<N>(val))` |
| Logical right shift | `shr(as_unsigned(val), amt)`            |

## Changes required

### Header (`z3wire/sym_bit_vec.h`)

- Remove `operator<<` and `operator>>` (both symbolic and mixed operand).
- Remove `checked_shl()` and `checked_shr()`.
- Rename `lossless_shl` overloads to `shl`.
- Add `shr(val, amt)` function.
- Add `as_signed(val)` and `as_unsigned(val)` functions.

### Tests (`z3wire/sym_bit_vec_test.cc`)

- Replace shift operator tests with `shl`/`shr` function call tests.
- Remove `checked_shl`/`checked_shr` tests.
- Add tests for `as_signed` and `as_unsigned`.

### Examples

- `examples/alu.cc` - Update if it uses shift operators.
- `examples/bit_manipulation.cc` - Uses `lossless_shl`, rename to `shl`.

### Documentation

- `README.md` - Update shift feature description.
- `docs/usage/operations.md` - Rewrite shift section.
- `docs/usage/cheatsheet.md` - Update shift table.
- `docs/design/overview.md` - Update shift section.
- `docs/examples/bit-manipulation.md` - Update code blocks.
- `docs/dev/test-coverage.md` - Update shift test references.
