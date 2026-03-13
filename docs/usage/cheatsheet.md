# Cheatsheet

Quick reference for the Z3Wire API. All types and functions live in the `z3w::`
namespace. Links point to detailed documentation.

## Types

| Symbolic | Concrete  | Description                    |
| :------- | :-------- | :----------------------------- |
| `Bool`   | `bool`    | Boolean                        |
| `Ubv<W>` | `UInt<W>` | Unsigned bit-vector of width W |
| `Sbv<W>` | `SInt<W>` | Signed bit-vector of width W   |

Concrete values auto-promote to symbolic in mixed expressions.
See [Types](types.md).

## Construction

```cpp
Ubv<8> x(ctx, "x");              // Symbolic variable
Bool b(ctx, "b");                 // Symbolic boolean

auto lit = Ubv<8>::Literal<255>(ctx);  // Compile-time checked literal
Bool::True(ctx);                       // Boolean literals
Bool::False(ctx);

UInt<8> c(42);                         // Concrete (masks to width)
auto [val, truncated] = UInt<8>::checked(999);  // Runtime checked
```

## Logic

| Op  | Expression | Width rule   |
| :-- | :--------- | :----------- |
| AND | `a && b`   | Bool only    |
| OR  | `a \|\| b` | Bool only    |
| NOT | `!a`       | Bool only    |
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

| Op                | Expression | Width rule                     |
| :---------------- | :--------- | :----------------------------- |
| `==` `!=`         | `a == b`   | Relaxed (any width/signedness) |
| `<` `<=` `>` `>=` | `a < b`    | Relaxed (any width/signedness) |

Returns `z3w::Bool`. Signedness-aware: uses signed comparison if either operand
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
bit<N>(val)                    // -> Ubv<1>            (single bit)
extract<High, Low>(val)        // -> Ubv<High-Low+1>  (static bounds)
extract<Width>(val, offset)    // -> Ubv<Width>        (symbolic offset)
concat(high, low)              // -> Ubv<W1+W2>
concat(a, b, c, ...)           // -> Ubv<W1+W2+W3+...> (variadic)
```

See [Operations](operations.md#bit-manipulation).

## Bit field

```cpp
bitfield_eq(buf, f1, f2, ...)  // -> Bool (LSB-first field equality constraint)
```

See [Operations](operations.md#bit-field).

## Casting

| Tier     | Expression             | Behavior                          |
| :------- | :--------------------- | :-------------------------------- |
| Hardware | `cast<T>(val)`         | Raw truncation/extension/bitcast  |
| Safe     | `safe_cast<T>(val)`    | Compile error if lossy            |
| Checked  | `checked_cast<T>(val)` | Returns `{result, overflow_flag}` |

See [Types](types.md#casting).

## Conversion

```cpp
to_bool(Ubv<1>)     // -> Bool
to_ubv1(Bool)        // -> Ubv<1>
```

## Conditional selection

```cpp
ite(cond, true_val, false_val)    // Both branches must be the same type
```

Works with `z3w::Bool` condition. Concrete values auto-promote to symbolic
in mixed expressions.

## Interop

```cpp
val.raw()     // Access underlying z3::expr (symbolic)
```

Concrete types use `.value()` to access the stored integer.
Users interact with `z3::context` and `z3::solver` directly.
