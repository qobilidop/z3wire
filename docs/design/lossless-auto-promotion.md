# Lossless Auto-Promotion

How Z3Wire decides which type conversions happen implicitly, which require
explicit casts, and why.

## Guiding principle

From the [north star](../dev/north_star.md):

> **No lossy implicit conversions:** Lossy conversions require explicit casts.
> Lossless conversions happen automatically.

A conversion from type A to type B is **lossless** if and only if every value
representable by A is also representable by B — that is, values(A) ⊆ values(B).

## Type tiers

Types that participate in Z3Wire operations fall into three tiers, ordered by
distance from Z3:

| Tier     | Types                                                                                              | Characteristics                                   |
| -------- | -------------------------------------------------------------------------------------------------- | ------------------------------------------------- |
| Native   | `bool`, signed integrals (`int`, `int64_t`, ...), unsigned integrals (`unsigned`, `uint64_t`, ...) | No Z3Wire awareness, fixed platform widths        |
| Concrete | `Bool`, `UInt<W>`, `SInt<W>`                                                                       | Z3Wire types, arbitrary compile-time width, no Z3 |
| Symbolic | `SymBool`, `SymUInt<W>`, `SymSInt<W>`                                                              | Wrap `z3::expr`, require `z3::context`            |

## Value sets

The losslessness criterion is value set inclusion. Each type's value set:

| Type                     | Value set                                              |
| ------------------------ | ------------------------------------------------------ |
| `bool`                   | {false, true}                                          |
| native signed (N bits)   | {−2^(N−1), ..., 2^(N−1) − 1}                           |
| native unsigned (N bits) | {0, ..., 2^N − 1}                                      |
| `Bool`                   | {false, true}                                          |
| `UInt<W>`                | {0, ..., 2^W − 1}                                      |
| `SInt<W>`                | {−2^(W−1), ..., 2^(W−1) − 1}                           |
| `SymBool`                | symbolic expressions over {false, true}                |
| `SymUInt<W>`             | symbolic expressions over {0, ..., 2^W − 1}            |
| `SymSInt<W>`             | symbolic expressions over {−2^(W−1), ..., 2^(W−1) − 1} |

Symbolic types represent the same value sets as their concrete counterparts —
they are just symbolic expressions over those sets rather than specific values.

## Promotion dimensions

Three orthogonal dimensions, plus one cross-kind conversion:

### 1. Concreteness (native → concrete → symbolic)

Adding Z3Wire wrapping or Z3 symbolic wrapping.

| Conversion                         | Lossless? | Condition      |
| ---------------------------------- | --------- | -------------- |
| native → concrete (matching width) | Yes       | Same value set |
| concrete → symbolic (same W, S)    | Yes       | Same value set |
| native → symbolic                  | Yes       | Via concrete   |

Concreteness promotion never changes the value set. It adds type safety (native
→ concrete) or symbolic reasoning capability (concrete → symbolic).

### 2. Width (W1 → W2)

Adding or removing bits.

| Conversion                       | Lossless? | Reason                      |
| -------------------------------- | --------- | --------------------------- |
| `UInt<W1>` → `UInt<W2>`, W2 ≥ W1 | Yes       | Zero-extend                 |
| `SInt<W1>` → `SInt<W2>`, W2 ≥ W1 | Yes       | Sign-extend                 |
| `UInt<W1>` → `SInt<W2>`, W2 > W1 | Yes       | Need one extra bit for sign |
| Any narrowing (W2 < W1)          | **No**    | High bits lost              |

### 3. Signedness (unsigned ↔ signed)

Reinterpreting the value domain at the same width.

| Conversion              | Lossless? | Reason                           |
| ----------------------- | --------- | -------------------------------- |
| `UInt<W>` → `SInt<W>`   | **No**    | Values ≥ 2^(W−1) unrepresentable |
| `SInt<W>` → `UInt<W>`   | **No**    | Negative values unrepresentable  |
| `UInt<W>` → `SInt<W+1>` | Yes       | Widening absorbs the sign bit    |

Same-width signed/unsigned conversion is always lossy. Safe conversion requires
widening by at least one bit (unsigned → signed) or is impossible in the other
direction without widening.

### 4. Kind (bool ↔ 1-bit vector)

| Conversion               | Lossless? | Reason                        |
| ------------------------ | --------- | ----------------------------- |
| `Bool` ↔ `UInt<1>`       | Yes       | Bijection: both have 2 values |
| `SymBool` ↔ `SymUInt<1>` | Yes       | Same bijection, symbolic      |

Although the value sets biject, these are semantically distinct types (logical
proposition vs hardware wire). Z3Wire requires explicit conversion via
`as_bool()` and `as_uint1()`.

## Current behavior

### What auto-promotes

| Context                            | What happens                                    | Promotion path           |
| ---------------------------------- | ----------------------------------------------- | ------------------------ |
| Mixed concrete/symbolic binary ops | Concrete side promoted to symbolic              | `to_symbolic(val, ctx)`  |
| Arithmetic with different widths   | Both sides widened, result has bit-growth width | Width rules per operator |

### What requires explicit conversion

| Conversion                  | API                        | Checking               |
| --------------------------- | -------------------------- | ---------------------- |
| Lossless cast (guaranteed)  | `safe_cast<T>(val)`        | Compile-time           |
| Checked cast (may truncate) | `checked_cast<T>(val)`     | Runtime (returns flag) |
| Raw hardware cast           | `unsafe_cast<T>(val)`      | None                   |
| Bool ↔ 1-bit vector         | `as_bool()` / `as_uint1()` | N/A (bijective)        |
| Concrete → symbolic         | `to_symbolic(val, ctx)`    | N/A (always lossless)  |

### What doesn't compile

| Conversion                                     | Why                                        |
| ---------------------------------------------- | ------------------------------------------ |
| Bitwise ops with mismatched widths             | Would silently change semantics            |
| Comparison with different widths or signedness | Operators are strict; use `math_*` or cast |
| `safe_cast` that would lose data               | `static_assert` fires                      |
| `safe_cast` signed → unsigned                  | Always forbidden (negative values)         |

## Considered and rejected

### `z3w::lit<N>()` — auto-width free function

Would return the narrowest `BitVec` fitting value N. Rejected because:

- Signedness is ambiguous. C++ integer literal `0xFF` has type `int` (signed),
    but a hardware programmer writing a mask almost certainly wants unsigned.
    Resolving this requires a special rule ("unsigned unless negative") that
    fights the language instead of working with it.
- Auto-width produces surprising types (`lit<0>()` → `UInt<1>`, `lit<256>()` →
    `UInt<9>`) that the programmer must reason about.
- Adds API surface without adding capability — `UInt<W>::Literal<V>()` already
    covers the need.

### `z3w::UIntLit<N>` / `z3w::SIntLit<N>` — named auto-width literals

Solves the signedness ambiguity (explicit in the name). Rejected because:

- Auto-width surprise remains (`UIntLit<0>` → `UInt<1>`).
- Adds a second way to construct literals alongside `Literal<V>()`.
- In contexts where width matters (bitwise ops), you still need
    `UInt<W>::Literal<V>()`.

### Concrete literals as the "shorter literal" solution

Not a new API — the realization that `UInt<16>::Literal<0x0800>()` (concrete)
works in place of `SymUInt<16>::Literal<0x0800>(ctx)` (symbolic) when used with
a symbolic operand in a mixed operation. The mixed-operand `operator==` promotes
the concrete side to symbolic automatically.

This eliminated the main pain point (redundant `ctx` parameter) without adding
API surface. Adopted and applied to examples.

## Open question: native integers in comparisons

Allow `ethertype == 0x0800` where `ethertype` is `SymUInt<16>`.

**Why it's interesting:** the native integer literal could convert losslessly
to a `BitVec` at the *symbolic operand's* width and signedness, after which
the existing mixed-operand `operator==` (strict on matching types) handles the
rest. Picking the symbolic operand's type for the conversion target keeps
operator semantics consistent: every comparison still has matching width and
signedness at the point of comparison.

**What the conversion has to check:** the native value must fit in the
target width and signedness without truncation. If it does, the conversion
is lossless; if it doesn't, the conversion is a compile error (for
constant-expression literals) or a runtime check (for non-constant values).
This is the same range-check `Literal<V>()` already performs.

**Scope:** Comparison operators only (`==`, `!=`, `<`, `<=`, `>`, `>=`).
Arithmetic and bitwise with raw integers would produce surprising result
types or width mismatches. Cross-type cases still go through `math_*`.

**Status:** Blocked on finalizing the auto-promotion strategy documented here.
The design intersects with the strict-comparison decision: the native-int
conversion must target the symbolic operand's type, not promote-then-widen.
Evaluate as part of a coherent promotion policy rather than piecemeal.
