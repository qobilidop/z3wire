# Casting

Z3Wire provides a three-tier casting API for symbolic types, giving you explicit
control over how bit-vectors change width or signedness.

## `unsafe_cast<T>(val)` - Hardware cast

Raw truncation, extension, or sign reinterpretation. No safety checks. Use when
you intentionally want hardware-style overflow or wrap behavior.

```cpp
z3w::SymUInt<16> wide(ctx, "wide");
auto narrow = z3w::unsafe_cast<z3w::SymUInt<8>>(wide);   // Truncate to 8 bits
auto wider = z3w::unsafe_cast<z3w::SymUInt<32>>(wide);    // Zero-extend to 32 bits
auto reint = z3w::unsafe_cast<z3w::SymSInt<16>>(wide);    // Reinterpret as signed
```

Under the hood:

| Conversion                  | Operation                                             |
| :-------------------------- | :---------------------------------------------------- |
| Target narrower than source | `z3::extract` (truncation)                            |
| Target wider than source    | `z3::zext` or `z3::sext` (based on source signedness) |
| Same width                  | Zero-overhead type reinterpretation                   |

## `safe_cast<T>(val)` - Compile-time guard

Only compiles if the cast is mathematically guaranteed to be lossless. Use when
you want the compiler to verify that no data can be lost.

```cpp
z3w::SymUInt<8> small(ctx, "small");
auto wide = z3w::safe_cast<z3w::SymUInt<16>>(small);   // OK: widening
auto bad = z3w::safe_cast<z3w::SymUInt<4>>(small);      // Compile error!
```

**Rules:**

| Source        | Target        | Allowed?                                       |
| :------------ | :------------ | :--------------------------------------------- |
| `SymUInt<W1>` | `SymUInt<W2>` | Yes, if `W2 >= W1`                             |
| `SymSInt<W1>` | `SymSInt<W2>` | Yes, if `W2 >= W1`                             |
| `SymUInt<W1>` | `SymSInt<W2>` | Yes, if `W2 > W1` (needs 1 extra bit for sign) |
| `SymSInt<W1>` | `SymUInt<W2>` | **Always forbidden** (negative values corrupt) |
| Any           | Smaller       | **Always forbidden** (truncation is not safe)  |

## `checked_cast<T>(val)` - Verification cast

Performs the cast and returns a symbolic boolean indicating whether the value
was preserved. Use when you want to verify safety as part of a Z3 proof.

```cpp
z3w::SymUInt<16> val(ctx, "val");
auto [result, value_preserved] = z3w::checked_cast<z3w::SymUInt<8>>(val);

// Assert in the solver: this cast must never lose data.
solver.add(value_preserved.expr());
```

The value-preservation flag works by round-tripping: cast to the target type,
cast back, and check if the value changed.

## Choosing the right tier

| Situation                                  | Use            |
| :----------------------------------------- | :------------- |
| Widening where data loss is impossible     | `safe_cast`    |
| Need to prove no data loss symbolically    | `checked_cast` |
| Modeling hardware truncation (intentional) | `unsafe_cast`  |

## Concrete-to-symbolic promotion

Concrete values automatically promote to symbolic in mixed expressions:

```cpp
z3w::SymUInt<8> sym(ctx, "x");
auto conc = z3w::UInt<8>::Literal<42>();

auto result = sym + conc;              // SymUInt<9>
z3w::SymBool eq = (sym == conc);       // Symbolic equality check
```

The promotion grabs the `z3::context` from the symbolic operand. To convert
manually:

```cpp
z3w::SymUInt<8> sym = z3w::to_symbolic(conc, ctx);
```

## SymBool / bit-vector conversion

In Z3, `SymBool` and a 1-bit bit-vector are distinct sorts. Hardware frequently
needs to convert between them. Z3Wire provides explicit conversion functions:

```cpp
// Bit-vector to SymBool
z3w::SymUInt<32> status(ctx, "status");
z3w::SymBool ready = z3w::as_bool(z3w::extract<0, 0>(status));

// SymBool to bit-vector
z3w::SymBool cond(ctx, "cond");
z3w::SymUInt<1> flag = z3w::as_uint1(cond);
```
