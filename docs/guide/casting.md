# Casting

Z3Wire provides a three-tier casting API, giving you explicit control over how
bit-vectors change width or signedness.

## `cast<T>(val)` — Hardware cast

Raw truncation, extension, or sign reinterpretation. No safety checks. Use when
you intentionally want hardware-style overflow or wrap behavior.

```cpp
z3w::Ubv<16> wide(ctx, "wide");
auto narrow = z3w::cast<z3w::Ubv<8>>(wide);   // Truncate to 8 bits
auto wider = z3w::cast<z3w::Ubv<32>>(wide);    // Zero-extend to 32 bits
auto reint = z3w::cast<z3w::Sbv<16>>(wide);    // Reinterpret as signed
```

Under the hood:

| Conversion | Operation |
|:-----------|:----------|
| Target narrower than source | `z3::extract` (truncation) |
| Target wider than source | `z3::zext` or `z3::sext` (based on source signedness) |
| Same width | Zero-overhead type reinterpretation |

## `safe_cast<T>(val)` — Compile-time guard

Only compiles if the cast is mathematically guaranteed to be lossless. Use when
you want the compiler to verify that no data can be lost.

```cpp
z3w::Ubv<8> small(ctx, "small");
auto wide = z3w::safe_cast<z3w::Ubv<16>>(small);   // OK: widening
auto bad = z3w::safe_cast<z3w::Ubv<4>>(small);      // Compile error!
```

**Rules:**

| Source | Target | Allowed? |
|:-------|:-------|:---------|
| `Ubv<W1>` | `Ubv<W2>` | Yes, if `W2 >= W1` |
| `Sbv<W1>` | `Sbv<W2>` | Yes, if `W2 >= W1` |
| `Ubv<W1>` | `Sbv<W2>` | Yes, if `W2 > W1` (needs 1 extra bit for sign) |
| `Sbv<W1>` | `Ubv<W2>` | **Always forbidden** (negative values corrupt) |
| Any | Smaller | **Always forbidden** (truncation is not safe) |

## `checked_cast<T>(val)` — Verification cast

Performs the cast and returns a symbolic boolean indicating whether data loss
occurred. Use when you want to verify safety as part of a Z3 proof.

```cpp
z3w::Ubv<16> val(ctx, "val");
auto [result, overflowed] = z3w::checked_cast<z3w::Ubv<8>>(val);

// Assert in the solver: this cast must never lose data.
solver.add(!overflowed.raw());
```

The overflow flag works by round-tripping: cast to the target type, cast back,
and check if the value changed.

## Choosing the right tier

| Situation | Use |
|:----------|:----|
| Modeling hardware truncation (intentional) | `cast` |
| Widening where data loss is impossible | `safe_cast` |
| Need to prove no data loss symbolically | `checked_cast` |
