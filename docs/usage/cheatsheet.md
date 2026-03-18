# Cheatsheet

Quick reference for the Z3Wire API. All types and functions live in the `z3w::`
namespace. Links point to detailed documentation.

## Types

| Symbolic     | Concrete  | Description                    |
| :----------- | :-------- | :----------------------------- |
| `SymBool`    | `Bool`    | Boolean                        |
| `SymUInt<W>` | `UInt<W>` | Unsigned bit-vector of width W |
| `SymSInt<W>` | `SInt<W>` | Signed bit-vector of width W   |

Concrete values auto-promote to symbolic in mixed expressions. See
[Types](types.md).

## Construction

```cpp
SymUInt<8> x(ctx, "x");              // Symbolic variable
SymBool b(ctx, "b");                  // Symbolic boolean

auto lit = SymUInt<8>::Literal<255>(ctx);  // Compile-time checked literal
SymBool::True(ctx);                        // Boolean literals
SymBool::False(ctx);

auto c = UInt<8>::Literal<42>();              // Compile-time checked concrete
auto [val, truncated] = UInt<8>::checked(999); // Runtime checked
```

## Logic

| Op  | Expression | Width rule   |
| :-- | :--------- | :----------- |
| AND | `a && b`   | SymBool only |
| OR  | `a \|\| b` | SymBool only |
| XOR | `a ^ b`    | SymBool only |
| NOT | `!a`       | SymBool only |
| AND | `a & b`    | Strict match |
| OR  | `a \| b`   | Strict match |
| XOR | `a ^ b`    | Strict match |
| NOT | `~a`       | Strict match |

See [Operations](operations.md).

## Arithmetic

| Op  | Expression | Result width      | Result signedness          |
| :-- | :--------- | :---------------- | :------------------------- |
| Add | `a + b`    | `max(W1, W2) + 1` | Signed if either is signed |
| Sub | `a - b`    | `max(W1, W2) + 1` | Always signed              |
| Neg | `-a`       | `W + 1`           | Always signed              |

Operands are auto-extended. Mixed widths and signedness allowed. See
[Operations](operations.md).

## Comparison

| Op                | Expression      | Width rule                     |
| :---------------- | :-------------- | :----------------------------- |
| `==` `!=`         | `a == b`        | Relaxed (any width/signedness) |
| `<` `<=` `>` `>=` | `a < b`         | Relaxed (any width/signedness) |
| Exact             | `exact_eq(a,b)` | Strict (same width/signedness) |

Returns `z3w::SymBool`. Signedness-aware: uses signed comparison if either
operand is signed. Concrete types support `==` and `!=` only (returning `bool`).
See [Operations](operations.md).

## Shifting

| Function        | Result width  |
| :-------------- | :------------ |
| `shl<N>(val)`   | `W + N`       |
| `shl(val, amt)` | `W + 2^K - 1` |
| `shr<N>(val)`   | `W`           |
| `shr(val, amt)` | `W`           |

- `shl` always returns `SymUInt` regardless of input signedness.
- `shr` is arithmetic (sign-preserving for signed types).
- `shr` amount is always `SymUInt<K>` (width may differ from value).
- For logical right shift on signed values: `shr(as_unsigned(val), amt)`.

See [Operations](operations.md#shifting).

## Bit manipulation

```cpp
bit<N>(val)                    // -> SymUInt<1>         (single bit)
extract<High, Low>(val)        // -> SymUInt<High-Low+1>  (static bounds)
extract<Width>(val, offset)    // -> SymUInt<Width>        (symbolic offset)
concat(high, low)              // -> SymUInt<W1+W2>
concat(a, b, c, ...)           // -> SymUInt<W1+W2+W3+...> (variadic)
```

See [Operations](operations.md#bit-manipulation).

## Casting

| Tier        | Expression             | Behavior                            |
| :---------- | :--------------------- | :---------------------------------- |
| Safe        | `safe_cast<T>(val)`    | Compile error if lossy              |
| Checked     | `checked_cast<T>(val)` | Returns `{result, value_preserved}` |
| Hardware    | `unsafe_cast<T>(val)`  | Raw truncation/extension/bitcast    |
| Reinterpret | `as_signed(val)`       | Same-width reinterpret to signed    |
| Reinterpret | `as_unsigned(val)`     | Same-width reinterpret to unsigned  |

See [Types](types.md#symbolic-casting).

## Conversion

```cpp
as_bool(SymUInt<1>)     // -> SymBool
as_uint1(SymBool)        // -> SymUInt<1>
```

## Conditional selection

```cpp
ite(cond, true_val, false_val)    // Both branches must be the same type
```

Works with `z3w::SymBool` condition. Concrete values auto-promote to symbolic in
mixed expressions.

## Interop

```cpp
val.expr()     // Access underlying z3::expr (symbolic)
```

Concrete types use `.bits()` (raw unsigned) and `.value()` (interpreted). Users
interact with `z3::context` and `z3::solver` directly.
