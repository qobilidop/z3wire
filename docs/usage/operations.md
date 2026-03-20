# Operations

Z3Wire operations work on symbolic types. Concrete values auto-promote to
symbolic in mixed expressions (see
[Type Conversions](type-conversions.md#to_symbolic-and-automatic-promotion)).

## Overview

| Category         | Operations                       | Operand rules               |
| :--------------- | :------------------------------- | :-------------------------- |
| Logical          | `&&`, `\|\|`, `^`, `!`           | `SymBool` only              |
| Bitwise          | `&`, `\|`, `^`, `~`              | Same width and signedness   |
| Comparison       | `==`, `!=`, `<`, `<=`, `>`, `>=` | Mixed allowed, auto-extends |
| Arithmetic       | `+`, `-`, unary `-`              | Mixed allowed, auto-extends |
| Shifting         | `shl`, `shr`                     | Any widths, `shl` widens    |
| Bit manipulation | `bit`, `extract`, `concat`       | Any widths                  |
| Mux              | `ite`                            | Same type                   |

All examples on this page assume a `z3::context ctx` is in scope.

## Logical

Standard logical operations on `SymBool`:

```cpp
z3w::SymBool a(ctx, "a");
z3w::SymBool b(ctx, "b");

z3w::SymBool c = a && b;   // AND
z3w::SymBool d = a || b;   // OR
z3w::SymBool e = a ^ b;    // XOR
z3w::SymBool f = !a;       // NOT
```

## Bitwise

Bitwise operations on bit-vectors. Operands must have the **exact same width and
signedness**; mismatches are compile errors.

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");

auto c = a & b;   // AND
auto d = a | b;   // OR
auto e = a ^ b;   // XOR
auto f = ~a;       // NOT
```

## Comparison

Comparison operations on bit-vectors. Mixed widths and signedness are allowed;
operands are automatically extended to a common type. Returns `SymBool`.

### Equality

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
z3w::SymBool eq = (a == b);
z3w::SymBool ne = (a != b);

// Mixed widths:
z3w::SymUInt<16> c(ctx, "c");
z3w::SymBool eq2 = (a == c);  // a is zero-extended to 16 bits first
```

Also works for `SymBool` operands.

### Ordered comparison

Automatically dispatches to unsigned or signed comparison based on the common
type (signed if either operand is signed).

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
z3w::SymBool less = (a < b);   // Unsigned comparison

z3w::SymSInt<8> x(ctx, "x");
z3w::SymSInt<8> y(ctx, "y");
z3w::SymBool less_s = (x < y);  // Signed comparison

// Mixed signedness:
z3w::SymBool less_m = (a < x);  // Common type: SymSInt<9>
```

## Arithmetic

Arithmetic operations on bit-vectors. Addition and subtraction automatically
widen the result type to prevent silent overflow. Mixed widths and signedness
are allowed; operands are automatically extended to a common type.

### Addition (`+`)

Result width = `max(W1, W2) + 1`. Result is signed if either operand is signed.

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
auto sum = a + b;       // SymUInt<9>
auto total = sum + a;   // SymUInt<10>

// Mixed widths:
z3w::SymUInt<16> c(ctx, "c");
auto sum2 = a + c;      // SymUInt<17>, a is zero-extended first
```

### Subtraction (`-`)

Result width = `max(W1, W2) + 1`. Result is always signed (subtraction can
produce negative values).

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
auto diff = a - b;  // SymSInt<9>
```

### Unary negate (`-`)

Result width = `W + 1`. Result is always signed. Consistent with subtraction
(`-x` is equivalent to `0 - x`).

```cpp
z3w::SymUInt<8> a(ctx, "a");
auto neg = -a;  // SymSInt<9>
```

!!! tip

    To truncate the result to a fixed width, use `unsafe_cast`:

    ```cpp
    auto reg = z3w::unsafe_cast<z3w::SymUInt<8>>(a + b);  // Truncate to 8 bits
    ```

## Shifting

Shift operations on bit-vectors. `shl` (left shift) always widens the result to
guarantee no bits are lost. `shr` (right shift) preserves the result width.

### `shl` - Left shift

Returns `SymUInt` regardless of input signedness.

**Constant shift** - result width = `W + N`:

```cpp
z3w::SymUInt<8> a(ctx, "a");
auto r = z3w::shl<3>(a);  // SymUInt<11>
```

**Symbolic shift** - result width = `W + 2^K - 1`, where `K` is the shift amount
width:

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<3> n(ctx, "n");    // Can shift by 0..7
auto r = z3w::shl(a, n);        // SymUInt<15> (8 + 2^3 - 1)
```

### `shr` - Right shift

Preserves sign for signed types (arithmetic shift), zero-fills for unsigned
types (logical shift). Result width stays the same.

**Constant shift**:

```cpp
z3w::SymUInt<8> a(ctx, "a");
auto r = z3w::shr<3>(a);  // SymUInt<8>
```

**Symbolic shift**:

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<3> n(ctx, "n");
auto r = z3w::shr(a, n);   // SymUInt<8>

z3w::SymSInt<8> x(ctx, "x");
auto r2 = z3w::shr(x, n);  // SymSInt<8>, sign bit preserved
```

### Composability

`shl`/`shr` compose with casting and signedness reinterpretation to cover common
shift patterns:

| Goal                          | Expression                              |
| :---------------------------- | :-------------------------------------- |
| Raw left shift (fixed width)  | `unsafe_cast<SymUInt<W>>(shl<N>(val))`  |
| Checked left shift (verify)   | `checked_cast<SymUInt<W>>(shl<N>(val))` |
| Logical right shift on signed | `shr(as_unsigned(val), amt)`            |

## Bit manipulation

Bit-level operations: extraction and concatenation.

### Single-bit extraction

Shorthand for extracting a single bit:

```cpp
z3w::SymUInt<32> x(ctx, "x");
auto sign = z3w::bit<31>(x);  // SymUInt<1>
auto lsb  = z3w::bit<0>(x);   // SymUInt<1>
```

Equivalent to `z3w::extract<N, N>(val)`.

### Extract (bit slicing)

**Static extract** - compile-time bounds, checked at compile time (`High >= Low`
and `High < W`):

```cpp
z3w::SymUInt<32> x(ctx, "x");
auto hi   = z3w::extract<31, 24>(x);  // SymUInt<8>
auto lo   = z3w::extract<3, 0>(x);    // SymUInt<4>
auto bit5 = z3w::extract<5, 5>(x);    // SymUInt<1>
```

!!! note

    The result is always `SymUInt` (unsigned). Extracted bits have no inherent
    signedness.

**Symbolic-offset extract** - fixed number of bits starting at a symbolic
position:

```cpp
z3w::SymUInt<32> x(ctx, "x");
z3w::SymUInt<5> offset(ctx, "offset");

auto nibble = z3w::extract<4>(x, offset);  // SymUInt<4>
auto byte   = z3w::extract<8>(x, offset);  // SymUInt<8>
```

Offsets beyond the source width yield zero bits (zero-extension semantics). The
index can be wider than the source bit-vector.

### Concatenation

Glue bit-vectors together. The result width is `W1 + W2`, always returned as
`SymUInt`.

```cpp
z3w::SymUInt<16> hi(ctx, "hi");
z3w::SymUInt<16> lo(ctx, "lo");
auto full = z3w::concat(hi, lo);  // SymUInt<32>
```

Supports variadic arguments:

```cpp
z3w::SymUInt<4> a(ctx, "a");
z3w::SymUInt<4> b(ctx, "b");
z3w::SymUInt<8> c(ctx, "c");
auto packed = z3w::concat(a, b, c);  // SymUInt<16>
```

## Mux

Symbolic If-Then-Else. Both branches must be the exact same type.

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
z3w::SymBool sel(ctx, "sel");

auto result = z3w::ite(sel, a, b);  // SymUInt<8>
```

Works with concrete values in mixed expressions:

```cpp
z3w::SymBool cond(ctx, "cond");
auto a = z3w::UInt<8>::Literal<42>();
auto b = z3w::UInt<8>::Literal<99>();

auto result = z3w::ite(cond, a, b);  // Promotes both to symbolic
```
