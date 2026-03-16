# Cheatsheet

Quick reference for the Z3Wire API. All types and functions live in the `z3w::`
namespace. Links point to detailed documentation.

## Types

| Symbolic     | Concrete  | Description                    |
| :----------- | :-------- | :----------------------------- |
| `SymBool`    | `Bool`    | Boolean                        |
| `SymUInt<W>` | `UInt<W>` | Unsigned bit-vector of width W |
| `SymSInt<W>` | `SInt<W>` | Signed bit-vector of width W   |

Concrete values auto-promote to symbolic in mixed expressions.
See [Types](types.md).

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

Operands are auto-extended. Mixed widths and signedness allowed.
See [Operations](operations.md).

## Comparison

| Op                | Expression      | Width rule                     |
| :---------------- | :-------------- | :----------------------------- |
| `==` `!=`         | `a == b`        | Relaxed (any width/signedness) |
| `<` `<=` `>` `>=` | `a < b`         | Relaxed (any width/signedness) |
| Exact             | `exact_eq(a,b)` | Strict (same width/signedness) |

Returns `z3w::SymBool`. Signedness-aware: uses signed comparison if either operand
is signed. Concrete types support `==` and `!=` only (returning `bool`).
See [Operations](operations.md).

## Shifting

| Tier     | Expression           | Result width            |
| :------- | :------------------- | :---------------------- |
| Hardware | `a << b`, `a >> b`   | Same (bits may be lost) |
| Checked  | `checked_shl(a, b)`  | Same + loss flag        |
| Checked  | `checked_shr(a, b)`  | Same + loss flag        |
| Lossless | `lossless_shl<N>(a)` | `W + N`                 |
| Lossless | `lossless_shl(a, n)` | `W + 2^K - 1`           |

Hardware shifts require strict width/signedness match. Right shift is logical
for unsigned, arithmetic for signed.
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

| Tier     | Expression             | Behavior                          |
| :------- | :--------------------- | :-------------------------------- |
| Hardware | `cast<T>(val)`         | Raw truncation/extension/bitcast  |
| Safe     | `safe_cast<T>(val)`    | Compile error if lossy            |
| Checked  | `checked_cast<T>(val)` | Returns `{result, overflow_flag}` |

See [Types](types.md#casting).

## Conversion

```cpp
to_bool(SymUInt<1>)     // -> SymBool
to_ubv1(SymBool)        // -> SymUInt<1>
```

## Conditional selection

```cpp
ite(cond, true_val, false_val)    // Both branches must be the same type
```

Works with `z3w::SymBool` condition. Concrete values auto-promote to symbolic
in mixed expressions.

## Interop

```cpp
val.raw()     // Access underlying z3::expr (symbolic)
```

Concrete types use `.bits()` (raw unsigned) and `.value()` (interpreted).
Users interact with `z3::context` and `z3::solver` directly.
