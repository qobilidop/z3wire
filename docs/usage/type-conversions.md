# Type Conversions

Z3Wire provides explicit APIs for converting between types: casting
(width/signedness), signedness reinterpretation, sort conversion, and switching
between symbolic and concrete.

## Overview

| Conversion                         | API                        | From → To           |
| :--------------------------------- | :------------------------- | :------------------ |
| Lossless widening                  | `safe_cast<T>`             | Symbolic → Symbolic |
| Verify preservation in solver      | `checked_cast<T>`          | Symbolic → Symbolic |
| Truncation, extension, reinterpret | `unsafe_cast<T>`           | Symbolic → Symbolic |
| Signedness reinterpretation        | `as_unsigned`, `as_signed` | Symbolic → Symbolic |
| Sort conversion                    | `as_bool`, `as_uint1`      | Symbolic → Symbolic |
| Concrete to symbolic               | `to_symbolic`, automatic   | Concrete → Symbolic |
| Symbolic to concrete               | `to_concrete`              | Symbolic → Concrete |

All examples on this page assume a `z3::context ctx` is in scope.

## Casting

A three-tier API for changing symbolic bit-vector width or signedness.

### `safe_cast<T>(val)` - Compile-time guard

Only compiles if the cast is mathematically guaranteed to be lossless:

```cpp
z3w::SymUInt<8> x(ctx, "x");
auto wide = z3w::safe_cast<z3w::SymUInt<16>>(x);  // OK: widening
auto bad  = z3w::safe_cast<z3w::SymUInt<4>>(x);   // Compile error!
```

**Rules:**

| Source        | Target        | Allowed?                                       |
| :------------ | :------------ | :--------------------------------------------- |
| `SymUInt<W1>` | `SymUInt<W2>` | Yes, if `W2 >= W1`                             |
| `SymSInt<W1>` | `SymSInt<W2>` | Yes, if `W2 >= W1`                             |
| `SymUInt<W1>` | `SymSInt<W2>` | Yes, if `W2 > W1` (needs 1 extra bit for sign) |
| `SymSInt<W1>` | `SymUInt<W2>` | **Always forbidden** (negative values corrupt) |
| Any           | Smaller       | **Always forbidden** (truncation is not safe)  |

### `checked_cast<T>(val)` - Verification cast

Performs the cast and returns a symbolic boolean indicating whether the value
was preserved. Use when you want to verify safety as part of a Z3 proof:

```cpp
z3::solver solver(ctx);
z3w::SymUInt<16> x(ctx, "x");
auto [narrow, preserved] = z3w::checked_cast<z3w::SymUInt<8>>(x);

// Assert in the solver: this cast must never lose data.
solver.add(preserved.expr());
```

The preservation flag works by round-tripping: cast to the target type, cast
back, and check if the value changed.

### `unsafe_cast<T>(val)` - Unchecked cast

Raw truncation, extension, or sign reinterpretation. No safety checks. Use when
you intentionally want overflow or wrap behavior:

```cpp
z3w::SymUInt<16> x(ctx, "x");
auto narrow = z3w::unsafe_cast<z3w::SymUInt<8>>(x);    // Truncate to 8 bits
auto wide   = z3w::unsafe_cast<z3w::SymUInt<32>>(x);   // Zero-extend to 32 bits
auto reint  = z3w::unsafe_cast<z3w::SymSInt<16>>(x);   // Reinterpret as signed
```

## Signedness reinterpretation

`as_unsigned` and `as_signed` reinterpret a symbolic bit-vector's signedness
without changing its width or value:

```cpp
z3w::SymSInt<8> x(ctx, "x");
z3w::SymUInt<8> y = z3w::as_unsigned(x);

z3w::SymUInt<8> a(ctx, "a");
z3w::SymSInt<8> b = z3w::as_signed(a);
```

These are convenience wrappers around `unsafe_cast` for the same-width case.

## Sort conversion

In Z3, `SymBool` and `SymUInt<1>` are distinct sorts. Converting between them
requires explicit functions:

```cpp
// SymUInt<1> to SymBool
z3w::SymUInt<1> x(ctx, "x");
z3w::SymBool b = z3w::as_bool(x);

// SymBool to SymUInt<1>
z3w::SymBool c(ctx, "c");
z3w::SymUInt<1> d = z3w::as_uint1(c);
```

## Concrete / symbolic conversion

### `to_symbolic` and automatic promotion

Concrete values automatically promote to symbolic in mixed expressions:

```cpp
z3w::SymUInt<8> x(ctx, "x");
auto y = z3w::UInt<8>::Literal<42>();

auto sum = x + y;                     // SymUInt<9>
z3w::SymBool eq = (x == y);           // Symbolic equality check
```

The promotion grabs the `z3::context` from the symbolic operand. To convert
manually:

```cpp
z3w::SymUInt<8> sym = z3w::to_symbolic(y, ctx);
z3w::SymBool sym_flag = z3w::to_symbolic(z3w::Bool(true), ctx);
```

### `to_concrete`

Extract a concrete value from a symbolic one using a solved model:

```cpp
z3::solver solver(ctx);
z3w::SymUInt<8> x(ctx, "x");
solver.add(x.expr() == 42);
solver.check();

z3w::UInt<8> result = z3w::to_concrete(x, solver.get_model());
// result.value() == 42
```
