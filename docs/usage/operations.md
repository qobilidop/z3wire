# Operations

Z3Wire operations work on symbolic types. Concrete values auto-promote to
symbolic in mixed expressions (see
[Type Conversions](type-conversions.md#to_symbolic-and-automatic-promotion)).

## Overview

| Category              | Operations                        |
| :-------------------- | :-------------------------------- |
| Logical               | `!`, `&&`, <code>\|\|</code>, `^` |
| Bitwise               | `~`, `&`, <code>\|</code>, `^`    |
| Comparison            | `==`, `!=`, `<`, `<=`, `>`, `>=`  |
| Arithmetic            | `+`, `-`, unary `-`               |
| Shifting              | `shl`, `shr`                      |
| Rotation              | `rotl`, `rotr`                    |
| Bit manipulation      | `extract`, `replace`, `concat`    |
| Conditional selection | `ite`                             |

All examples on this page assume a `z3::context ctx` is in scope.

## Logical

Logical operations for `SymBool`.

Unary operator typing rule:

| Operation | Operand     | Syntax | Result type |
| :-------- | :---------- | :----- | :---------- |
| NOT       | `SymBool a` | `!a`   | `SymBool`   |

Binary operator typing rules:

| Operation | LHS         | RHS         | Syntax                | Result type |
| :-------- | :---------- | :---------- | :-------------------- | :---------- |
| AND       | `SymBool a` | `SymBool b` | `a && b`              | `SymBool`   |
| OR        | `SymBool a` | `SymBool b` | <code>a \|\| b</code> | `SymBool`   |
| XOR       | `SymBool a` | `SymBool b` | `a ^ b`               | `SymBool`   |

Examples:

```cpp
z3w::SymBool a(ctx, "a");
z3w::SymBool b(ctx, "b");

z3w::SymBool r1 = !a;
z3w::SymBool r2 = a && b;
z3w::SymBool r3 = a || b;
z3w::SymBool r4 = a ^ b;
```

## Bitwise

Bitwise operations for `SymUInt` and `SymSInt`. The `SymSInt` versions are
purely for convenience. The operations work on the underlying bits regardless of
the signedness.

Unary operator typing rules:

| Operation | Operand        | Syntax | Result type  |
| :-------- | :------------- | :----- | :----------- |
| NOT       | `SymUInt<W> a` | `~a`   | `SymUInt<W>` |
| NOT       | `SymSInt<W> a` | `~a`   | `SymSInt<W>` |

Binary operator typing rules (operand types must be exactly the same):

| Operation | LHS            | RHS            | Syntax              | Result type  |
| :-------- | :------------- | :------------- | :------------------ | :----------- |
| AND       | `SymUInt<W> a` | `SymUInt<W> b` | `a & b`             | `SymUInt<W>` |
| AND       | `SymSInt<W> a` | `SymSInt<W> b` | `a & b`             | `SymSInt<W>` |
| OR        | `SymUInt<W> a` | `SymUInt<W> b` | <code>a \| b</code> | `SymUInt<W>` |
| OR        | `SymSInt<W> a` | `SymSInt<W> b` | <code>a \| b</code> | `SymSInt<W>` |
| XOR       | `SymUInt<W> a` | `SymUInt<W> b` | `a ^ b`             | `SymUInt<W>` |
| XOR       | `SymSInt<W> a` | `SymSInt<W> b` | `a ^ b`             | `SymSInt<W>` |

Examples:

```cpp
z3w::SymUInt<8> u8(ctx, "u8");
z3w::SymSInt<8> s8(ctx, "s8");

z3w::SymUInt<8> r1 = ~u8;
z3w::SymSInt<8> r2 = ~s8;

z3w::SymUInt<8> r3 = u8 & u8;
z3w::SymSInt<8> r4 = s8 & s8;

z3w::SymUInt<8> r5 = u8 | u8;
z3w::SymSInt<8> r6 = s8 | s8;

z3w::SymUInt<8> r7 = u8 ^ u8;
z3w::SymSInt<8> r8 = s8 ^ s8;
```

## Comparison

### Boolean comparison

Comparison operations for `SymBool`.

Typing rules:

| Operation    | LHS         | RHS         | Syntax   | Result type |
| :----------- | :---------- | :---------- | :------- | :---------- |
| Equal to     | `SymBool a` | `SymBool b` | `a == b` | `SymBool`   |
| Not equal to | `SymBool a` | `SymBool b` | `a != b` | `SymBool`   |

Examples:

```cpp
z3w::SymBool a(ctx, "a");
z3w::SymBool b(ctx, "b");

z3w::SymBool eq = (a == b);
z3w::SymBool ne = (a != b);
```

### Integer comparison

Comparison operators for `SymUInt` and `SymSInt` are strict: both operands must
have the same width and signedness. For comparisons across types, use the
`z3w::math_*` free-function family — it asks the mathematical (value-based)
question, regardless of representation.

Typing rules:

| Operation                | LHS            | RHS            | Syntax          | Result type |
| :----------------------- | :------------- | :------------- | :-------------- | :---------- |
| Equal to                 | `SymUInt<A> a` | `SymUInt<A> b` | `a == b`        | `SymBool`   |
| Not equal to             | `SymUInt<A> a` | `SymUInt<A> b` | `a != b`        | `SymBool`   |
| Greater than             | `SymUInt<A> a` | `SymUInt<A> b` | `a > b`         | `SymBool`   |
| Greater than or equal to | `SymUInt<A> a` | `SymUInt<A> b` | `a >= b`        | `SymBool`   |
| Less than                | `SymUInt<A> a` | `SymUInt<A> b` | `a < b`         | `SymBool`   |
| Less than or equal to    | `SymUInt<A> a` | `SymUInt<A> b` | `a <= b`        | `SymBool`   |
| Mathematical equal       | `SymUInt<A> a` | `SymSInt<B> b` | `math_eq(a, b)` | `SymBool`   |
| Mathematical not equal   | `SymUInt<A> a` | `SymSInt<B> b` | `math_ne(a, b)` | `SymBool`   |
| Mathematical greater     | `SymUInt<A> a` | `SymSInt<B> b` | `math_gt(a, b)` | `SymBool`   |
| Mathematical greater-eq  | `SymUInt<A> a` | `SymSInt<B> b` | `math_ge(a, b)` | `SymBool`   |
| Mathematical less        | `SymUInt<A> a` | `SymSInt<B> b` | `math_lt(a, b)` | `SymBool`   |
| Mathematical less-eq     | `SymUInt<A> a` | `SymSInt<B> b` | `math_le(a, b)` | `SymBool`   |

The `math_*` functions accept any combination of `SymUInt` / `SymSInt` widths
and signednesses, and concrete `UInt` / `SInt` literals are accepted on either
side.

Examples:

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");

// Strict operators: same type required.
z3w::SymBool eq = (a == b);
z3w::SymBool lt = (a < b);

// Heterogeneous mathematical comparison.
z3w::SymSInt<16> c(ctx, "c");
z3w::SymBool eq_math = z3w::math_eq(a, c);
z3w::SymBool lt_math = z3w::math_lt(a, c);

// Overflow check: compare a wider sum against an 8-bit threshold.
auto sum = a + b;  // SymUInt<9>
z3w::SymBool overflow = z3w::math_gt(sum, z3w::UInt<8>::Literal<255>());
```

## Arithmetic

Arithmetic operations for `SymUInt` and `SymSInt` with bit-growth semantics.
Result types are automatically derived at compile time to be the minimal type
that could hold all possible results losslessly.

### Addition

Syntax: `a + b`

Typing rules (same as CIRCT
[`hwarith.add`](https://circt.llvm.org/docs/Dialects/HWArith/#hwarithadd-circthwarithaddop)):

| LHS            | RHS            | Result type                 |
| :------------- | :------------- | :-------------------------- |
| `SymUInt<A> a` | `SymUInt<B> b` | `SymUInt<max(A,B)+1>`       |
| `SymSInt<A> a` | `SymSInt<B> b` | `SymSInt<max(A,B)+1>`       |
| `SymUInt<A> a` | `SymSInt<B> b` | `SymSInt<A+2>`, if `A >= B` |
|                |                | `SymSInt<B+1>`, if `A < B`  |
| `SymSInt<A> a` | `SymUInt<B> b` | Same type as `b + a`        |

Examples:

```cpp
z3w::SymUInt<7> u7(ctx, "u7");
z3w::SymUInt<9> u9(ctx, "u9");
z3w::SymSInt<7> s7(ctx, "s7");
z3w::SymSInt<9> s9(ctx, "s9");

z3w::SymUInt<10> r1 = u7 + u9;
z3w::SymSInt<10> r2 = s7 + s9;

z3w::SymSInt<11> r3 = u9 + s7;
z3w::SymSInt<10> r4 = u7 + s9;

z3w::SymSInt<11> r5 = s7 + u9;
z3w::SymSInt<10> r6 = s9 + u7;
```

### Subtraction

Syntax: `a - b`

Typing rules (same as CIRCT
[`hwarith.sub`](https://circt.llvm.org/docs/Dialects/HWArith/#hwarithsub-circthwarithsubop)):

| LHS            | RHS            | Result type                 |
| :------------- | :------------- | :-------------------------- |
| `SymUInt<A> a` | `SymUInt<B> b` | `SymSInt<max(A,B)+1>`       |
| `SymSInt<A> a` | `SymSInt<B> b` | `SymSInt<max(A,B)+1>`       |
| `SymUInt<A> a` | `SymSInt<B> b` | `SymSInt<A+2>`, if `A >= B` |
|                |                | `SymSInt<B+1>`, if `A < B`  |
| `SymSInt<A> a` | `SymUInt<B> b` | Same type as `b - a`        |

Examples:

```cpp
z3w::SymUInt<7> u7(ctx, "u7");
z3w::SymUInt<9> u9(ctx, "u9");
z3w::SymSInt<7> s7(ctx, "s7");
z3w::SymSInt<9> s9(ctx, "s9");

z3w::SymSInt<10> r1 = u7 - u9;
z3w::SymSInt<10> r2 = s7 - s9;

z3w::SymSInt<11> r3 = u9 - s7;
z3w::SymSInt<10> r4 = u7 - s9;

z3w::SymSInt<11> r5 = s7 - u9;
z3w::SymSInt<10> r6 = s9 - u7;
```

### Arithmetic negation

Syntax: `-a`

Typing rules:

| Operand        | Result type    |
| :------------- | :------------- |
| `SymUInt<A> a` | `SymSInt<A+1>` |
| `SymSInt<A> a` | `SymSInt<A+1>` |

Examples:

```cpp
z3w::SymUInt<8> u8(ctx, "u8");
z3w::SymSInt<8> s8(ctx, "s8");

z3w::SymSInt<9> r1 = -u8;
z3w::SymSInt<9> r2 = -s8;
```

## Shifting

`shl` always widens the result to guarantee no bits are lost and always returns
`SymUInt` regardless of input signedness. `shr` preserves the result width and
signedness.

### Left shift

Left shift for `SymUInt` and `SymSInt` with constant or symbolic amount.

Typing rules:

| Value to shift | Shift amount   | Syntax      | Result type        |
| :------------- | :------------- | :---------- | :----------------- |
| `SymUInt<W> a` | `size_t N`     | `shl<N>(a)` | `SymUInt<W+N>`     |
| `SymUInt<W> a` | `SymUInt<K> n` | `shl(a, n)` | `SymUInt<W+2^K-1>` |
| `SymSInt<W> a` | `size_t N`     | `shl<N>(a)` | `SymUInt<W+N>`     |
| `SymSInt<W> a` | `SymUInt<K> n` | `shl(a, n)` | `SymUInt<W+2^K-1>` |

Examples:

```cpp
z3w::SymUInt<8> u8(ctx, "u8");
z3w::SymSInt<8> s8(ctx, "s8");
z3w::SymUInt<3> n(ctx, "n");

z3w::SymUInt<11> r1 = z3w::shl<3>(u8);
z3w::SymUInt<15> r2 = z3w::shl(u8, n);
z3w::SymUInt<11> r3 = z3w::shl<3>(s8);
z3w::SymUInt<15> r4 = z3w::shl(s8, n);
```

### Arithmetic right shift

Arithmetic right shift for `SymUInt` and `SymSInt` with constant or symbolic
amount.

Typing rules:

| Value to shift | Shift amount   | Syntax      | Result type  |
| :------------- | :------------- | :---------- | :----------- |
| `SymUInt<W> a` | `size_t N`     | `shr<N>(a)` | `SymUInt<W>` |
| `SymUInt<W> a` | `SymUInt<K> n` | `shr(a, n)` | `SymUInt<W>` |
| `SymSInt<W> a` | `size_t N`     | `shr<N>(a)` | `SymSInt<W>` |
| `SymSInt<W> a` | `SymUInt<K> n` | `shr(a, n)` | `SymSInt<W>` |

Examples:

```cpp
z3w::SymUInt<8> u8(ctx, "u8");
z3w::SymSInt<8> s8(ctx, "s8");
z3w::SymUInt<3> n(ctx, "n");

z3w::SymUInt<8> r1 = z3w::shr<3>(u8);
z3w::SymUInt<8> r2 = z3w::shr(u8, n);
z3w::SymSInt<8> r3 = z3w::shr<3>(s8);
z3w::SymSInt<8> r4 = z3w::shr(s8, n);
```

### Logical right shift

Simply do:

- `shr<N>(as_unsigned(a))`
- `shr(as_unsigned(a), n)`

## Rotation

Bit rotation (circular shift). The result preserves the input type and width.
Rotation amounts wrap modulo the bit width.

### Left rotation

Typing rules:

| Value to rotate | Rotation amount | Syntax       | Result type  |
| :-------------- | :-------------- | :----------- | :----------- |
| `SymUInt<W> a`  | `size_t N`      | `rotl<N>(a)` | `SymUInt<W>` |
| `SymUInt<W> a`  | `SymUInt<K> n`  | `rotl(a, n)` | `SymUInt<W>` |
| `SymSInt<W> a`  | `size_t N`      | `rotl<N>(a)` | `SymSInt<W>` |
| `SymSInt<W> a`  | `SymUInt<K> n`  | `rotl(a, n)` | `SymSInt<W>` |

Examples:

```cpp
z3w::SymUInt<8> u8(ctx, "u8");
z3w::SymSInt<8> s8(ctx, "s8");
z3w::SymUInt<3> n(ctx, "n");

z3w::SymUInt<8> r1 = z3w::rotl<3>(u8);
z3w::SymUInt<8> r2 = z3w::rotl(u8, n);
z3w::SymSInt<8> r3 = z3w::rotl<3>(s8);
z3w::SymSInt<8> r4 = z3w::rotl(s8, n);
```

### Right rotation

Typing rules:

| Value to rotate | Rotation amount | Syntax       | Result type  |
| :-------------- | :-------------- | :----------- | :----------- |
| `SymUInt<W> a`  | `size_t N`      | `rotr<N>(a)` | `SymUInt<W>` |
| `SymUInt<W> a`  | `SymUInt<K> n`  | `rotr(a, n)` | `SymUInt<W>` |
| `SymSInt<W> a`  | `size_t N`      | `rotr<N>(a)` | `SymSInt<W>` |
| `SymSInt<W> a`  | `SymUInt<K> n`  | `rotr(a, n)` | `SymSInt<W>` |

Examples:

```cpp
z3w::SymUInt<8> u8(ctx, "u8");
z3w::SymSInt<8> s8(ctx, "s8");
z3w::SymUInt<3> n(ctx, "n");

z3w::SymUInt<8> r1 = z3w::rotr<3>(u8);
z3w::SymUInt<8> r2 = z3w::rotr(u8, n);
z3w::SymSInt<8> r3 = z3w::rotr<3>(s8);
z3w::SymSInt<8> r4 = z3w::rotr(s8, n);
```

## Bit manipulation

All results are `SymUInt` regardless of input signedness.

### Static extraction

Both the width and the low-bit offset are template parameters; both are checked
at compile time.

Typing rules:

| Source           | Width      | Low offset  | Syntax                | Result type  | Compile-time checks    |
| :--------------- | :--------- | :---------- | :-------------------- | :----------- | :--------------------- |
| `SymUInt<W> src` | `size_t T` | `size_t Lo` | `extract<T, Lo>(src)` | `SymUInt<T>` | `T > 0 && Lo + T <= W` |
| `SymSInt<W> src` | `size_t T` | `size_t Lo` | `extract<T, Lo>(src)` | `SymUInt<T>` | `T > 0 && Lo + T <= W` |

Examples:

```cpp
z3w::SymUInt<32> src(ctx, "src");

z3w::SymUInt<8> hi = z3w::extract<8, 24>(src);    // bits [31:24]
z3w::SymUInt<4> lo = z3w::extract<4, 0>(src);     // bits [3:0]
z3w::SymUInt<1> bit5 = z3w::extract<1, 5>(src);   // bit 5
```

### Runtime-offset extraction

The width is a template parameter; the low-bit offset is a runtime `size_t`. The
width is checked at compile time; the offset is checked at runtime via
`Z3W_CHECK`, which aborts on out-of-range access.

Typing rules:

| Source           | Width      | Low offset  | Syntax                | Result type  | Compile-time checks | Runtime check |
| :--------------- | :--------- | :---------- | :-------------------- | :----------- | :------------------ | :------------ |
| `SymUInt<W> src` | `size_t T` | `size_t lo` | `extract<T>(src, lo)` | `SymUInt<T>` | `T > 0 && T <= W`   | `lo + T <= W` |
| `SymSInt<W> src` | `size_t T` | `size_t lo` | `extract<T>(src, lo)` | `SymUInt<T>` | `T > 0 && T <= W`   | `lo + T <= W` |

Examples:

```cpp
z3w::SymUInt<32> src(ctx, "src");

for (size_t i = 0; i < 4; ++i) {
  z3w::SymUInt<8> byte = z3w::extract<8>(src, i * 8);
}
```

### Symbolic-offset extraction

There is no dedicated symbolic-offset overload. Compose `shr` with a static
zero-offset extract. The chosen out-of-bounds semantic is visible at the call
site.

In the table below, `u` is a `SymUInt<SrcW>`, `s` is a `SymSInt<SrcW>`, `v` is
either, `idx` is a `SymUInt<K>`, `W` is the target width, and `ctx` is the Z3
context.

| Desired OOB semantic    | Expression                                                                             | Notes                                 |
| :---------------------- | :------------------------------------------------------------------------------------- | :------------------------------------ |
| Zero-fill               | `extract<W, 0>(shr(u, idx))`                                                           | `shr` uses `lshr` for unsigned `u`.   |
| Sign-extend             | `extract<W, 0>(shr(s, idx))`                                                           | `shr` uses `ashr` for signed `s`.     |
| Zero-fill, signed `src` | `extract<W, 0>(shr(as_unsigned(s), idx))`                                              | Cast first to opt into logical shift. |
| Asserted via solver     | `extract<W, 0>(shr(v, idx))` + `solver.add(idx <= SymUInt<K>::Literal<SrcW - W>(ctx))` | Caller forbids OOB at proof time.     |

Wraparound semantics (`idx mod SrcW`) require modular reduction, which Z3Wire
does not yet expose. Drop to a raw `z3::urem` if needed, or wait for the
operation to land (see roadmap).

### Static replacement

Both the source position and the field are compile-time-fixed.

Typing rules:

| Source            | Replacement field    | Low bit offset | Syntax                | Result type   | Compile-time checks |
| :---------------- | :------------------- | :------------- | :-------------------- | :------------ | :------------------ |
| `SymUInt<WS> src` | `SymBitVec<WF,SF> f` | `size_t LO`    | `replace<LO>(src, f)` | `SymUInt<WS>` | `LO + WF <= WS`     |
| `SymSInt<WS> src` | `SymBitVec<WF,SF> f` | `size_t LO`    | `replace<LO>(src, f)` | `SymSInt<WS>` | `LO + WF <= WS`     |

The field may have any signedness; its bits are placed as-is.

Examples:

```cpp
z3w::SymUInt<32> u32(ctx, "u32");
z3w::SymSInt<32> s32(ctx, "s32");
z3w::SymUInt<8> field(ctx, "field");

z3w::SymUInt<32> r1 = z3w::replace<13>(u32, field);
z3w::SymSInt<32> r2 = z3w::replace<13>(s32, field);
```

### Runtime-offset replacement

The field width is compile-time; the low-bit offset is a runtime `size_t`. The
width is checked at compile time; the offset is checked at runtime via
`Z3W_CHECK`, which aborts on out-of-range access.

Typing rules:

| Source            | Replacement field    | Low bit offset | Syntax                | Result type   | Compile-time checks | Runtime check   |
| :---------------- | :------------------- | :------------- | :-------------------- | :------------ | :------------------ | :-------------- |
| `SymUInt<WS> src` | `SymBitVec<WF,SF> f` | `size_t lo`    | `replace(src, f, lo)` | `SymUInt<WS>` | `WF <= WS`          | `lo + WF <= WS` |
| `SymSInt<WS> src` | `SymBitVec<WF,SF> f` | `size_t lo`    | `replace(src, f, lo)` | `SymSInt<WS>` | `WF <= WS`          | `lo + WF <= WS` |

Examples:

```cpp
z3w::SymUInt<32> src(ctx, "src");
z3w::SymUInt<8> field(ctx, "field");

for (size_t i = 0; i < 4; ++i) {
  z3w::SymUInt<32> updated = z3w::replace(src, field, i * 8);
}
```

### Symbolic-offset replacement

The low-bit offset is a symbolic value. The result is defined for every solver
model: field bits placed beyond the source width have no effect.

Typing rules:

| Source            | Replacement field    | Low bit offset   | Syntax                | Result type   | Compile-time checks |
| :---------------- | :------------------- | :--------------- | :-------------------- | :------------ | :------------------ |
| `SymUInt<WS> src` | `SymBitVec<WF,SF> f` | `SymUInt<WL> lo` | `replace(src, f, lo)` | `SymUInt<WS>` | `WF <= WS`          |
| `SymSInt<WS> src` | `SymBitVec<WF,SF> f` | `SymUInt<WL> lo` | `replace(src, f, lo)` | `SymSInt<WS>` | `WF <= WS`          |

Examples:

```cpp
z3w::SymUInt<32> u32(ctx, "u32");
z3w::SymSInt<32> s32(ctx, "s32");
z3w::SymUInt<8> field(ctx, "field");
z3w::SymUInt<5> lo(ctx, "lo");

z3w::SymUInt<32> r1 = z3w::replace(u32, field, lo);
z3w::SymSInt<32> r2 = z3w::replace(s32, field, lo);
```

Callers who want strict semantics (field must fit) can add a solver constraint:

```cpp
solver.add(lo <= z3w::SymUInt<5>::Literal<32 - 8>(ctx));
```

Unlike `extract`, the symbolic-offset form of `replace` is kept rather than
removed in favor of explicit composition. The composition pattern for `replace`
is ~8 lines with subtle correctness traps (`shl` widens its result and must be
truncated back to source width, the all-ones mask construction has a `WF < 64`
ceiling, signedness juggling around the bitwise ops). The OOB-policy menu is
also narrower: silent-discard is the only choice with a clear hardware analogue,
so there's no benefit to making the choice explicit at the call site.

### Concatenation

Concatenate a variadic number of symbolic inputs (`SymUInt`, `SymSInt`, or
`SymBool`) into a single `SymUInt`. The field packing order is always from MSB
to LSB.

Syntax: `concat(a, b, ...)`

Typing rules:

- Result type is always `SymUInt<W>`, where `W` is the sum of inputs' bit
    widths. `SymBool` counts as 1 bit.

Examples:

```cpp
z3w::SymBool b(ctx, "b");
z3w::SymUInt<7> u7(ctx, "u7");
z3w::SymSInt<8> s8(ctx, "s8");

z3w::SymUInt<8> r1 = z3w::concat(b, u7);
z3w::SymUInt<16> r2 = z3w::concat(b, u7, s8);
```

## Conditional selection

Symbolic if-then-else. Selects between two values based on a `SymBool`
condition.

Syntax: `ite(sel, a, b)`

Typing rules (choice types must be exactly the same):

| Condition     | Choice A       | Choice B       | Result type  |
| :------------ | :------------- | :------------- | :----------- |
| `SymBool sel` | `SymBool a`    | `SymBool b`    | `SymBool`    |
| `SymBool sel` | `SymUInt<W> a` | `SymUInt<W> b` | `SymUInt<W>` |
| `SymBool sel` | `SymSInt<W> a` | `SymSInt<W> b` | `SymSInt<W>` |

Examples:

```cpp
z3w::SymBool sel(ctx, "sel");
z3w::SymBool a1(ctx, "a1");
z3w::SymBool b1(ctx, "b1");
z3w::SymUInt<8> a2(ctx, "a2");
z3w::SymUInt<8> b2(ctx, "b2");
z3w::SymSInt<8> a3(ctx, "a3");
z3w::SymSInt<8> b3(ctx, "b3");

z3w::SymBool r1 = z3w::ite(sel, a1, b1);
z3w::SymUInt<8> r2 = z3w::ite(sel, a2, b2);
z3w::SymSInt<8> r3 = z3w::ite(sel, a3, b3);
```
