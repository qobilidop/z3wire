# Three-Tier Casting

Why Z3Wire has three cast functions (`safe_cast`, `checked_cast`, `unsafe_cast`)
instead of one, and when to use each.

## The decision

Width and signedness conversions use a three-tier API:

| Tier            | Function          | Checking        | Fails when                       |
| --------------- | ----------------- | --------------- | -------------------------------- |
| Compile-time    | `safe_cast<T>`    | `static_assert` | Cast could lose data             |
| Solver-verified | `checked_cast<T>` | Symbolic flag   | Value not preserved (per solver) |
| Unchecked       | `unsafe_cast<T>`  | None            | Never (silently truncates)       |

For the complete rules and examples, see the
[Type Conversions](../usage/type-conversions.md#casting) usage page.

## Why three tiers

### The problem: not all casts are equal

In verification code, a type cast can mean very different things:

1. "This widening is always safe — the compiler should verify it."
1. "This narrowing might lose data — I want to ask the solver about it."
1. "I know this truncates — it's intentional hardware behavior."

A single cast function can't serve all three intents. If it's permissive (like
C++ `static_cast`), safe casts get no compile-time protection. If it's strict
(like a checked cast), intentional truncation becomes cumbersome.

### Each tier serves a distinct intent

**`safe_cast`** — "I believe this is lossless. Prove it at compile time."

The compiler verifies via `static_assert` that the source value set is a subset
of the target value set. If it isn't, the code doesn't compile. This is for
structural guarantees — widening, or casting between types where losslessness is
obvious from the types alone.

```cpp
auto wide = z3w::safe_cast<z3w::SymUInt<16>>(narrow_8bit);  // always OK
auto bad  = z3w::safe_cast<z3w::SymUInt<4>>(narrow_8bit);   // compile error
```

**`checked_cast`** — "This might lose data. Let me verify it in the solver."

Returns `{result, value_preserved}` where `value_preserved` is a `SymBool`. The
programmer adds this flag to the solver to verify that truncation doesn't
actually occur for the inputs that matter. This is the verification-native
alternative to runtime overflow checks.

```cpp
auto [narrow, ok] = z3w::checked_cast<z3w::SymUInt<8>>(wide_9bit);
solver.add(ok.expr());  // assert: no overflow in this proof
```

**`unsafe_cast`** — "I want exactly these bits. Don't check anything."

Raw truncation, extension, or bitcast. The name signals intent: this is a
deliberate bit-level operation, not an accidental data loss. Used when modeling
hardware behavior that intentionally wraps or reinterprets.

```cpp
// Model hardware: 8-bit register wraps on overflow
auto hw_sum = z3w::unsafe_cast<z3w::SymUInt<8>>(a + b);
```

## Why `safe_cast` forbids signed-to-unsigned

`safe_cast<SymUInt<W>>(signed_value)` is always a compile error, regardless of
width. Even `safe_cast<SymUInt<64>>(SymSInt<8>)` — widening from 8 to 64 bits —
is forbidden.

The reason: a signed type can represent negative values, and no unsigned type
(at any width) can represent negative values. The value set of `SInt<W>` is
never a subset of `UInt<W2>` for any W2, because it includes negative numbers.

This is conservative by design. The programmer might know that the value is
non-negative in practice, but `safe_cast` only reasons about types, not values.
For value-dependent safety, use `checked_cast` and let the solver verify it. For
intentional reinterpretation, use `unsafe_cast`.

## How `checked_cast` works

The implementation is a round-trip test:

1. Cast the source to the target type (`unsafe_cast`).
1. Cast the result back to the source type (`unsafe_cast`).
1. Compare the round-tripped value to the original.

If the round-trip produces the same value, no information was lost. The
comparison is a symbolic equality check — it becomes a constraint the solver can
reason about.

```
value_preserved = (unsafe_cast<Source>(unsafe_cast<Target>(val)) == val)
```

This is elegant because it works for any combination of width and signedness
changes, and the solver handles all the edge cases (sign extension, truncation
of negative values, etc.) automatically.

## Connection to bit-growth arithmetic

The three tiers form a natural workflow with
[bit-growth arithmetic](bit-growth-arithmetic.md):

1. **Compute** — bit-growth produces the true, overflow-free result.
1. **Narrow** — `checked_cast` truncates to hardware width, flagging overflow.
1. **Model hardware** — `unsafe_cast` truncates without checking, when
    intentional wrap behavior is needed.

`safe_cast` is typically used in the other direction — widening a value before
an operation, or converting between types in a lossless pipeline.

## Connection to auto-promotion

The casting tiers complement the automatic promotions described in
[Lossless Auto-Promotion](lossless-auto-promotion.md):

- **Auto-promotion** handles the common case: concrete→symbolic, mixed-width
    comparisons, and arithmetic widening happen without explicit casts.
- **Casting** handles the deliberate case: when the programmer wants to change a
    type and needs to choose the appropriate safety level.

The two systems don't overlap — auto-promotion never narrows or changes
signedness, and casting is always explicit.
