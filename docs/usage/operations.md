# Operations

Z3Wire operations work on symbolic types. Concrete values auto-promote to
symbolic in mixed expressions (see
[Type Conversions](type-conversions.md#to_symbolic-and-automatic-promotion)).

## Overview

| Category              | Operations                       |
| :-------------------- | :------------------------------- |
| Logical               | `!`, `&&`, `\|\|`, `^`           |
| Bitwise               | `~`, `&`, `\|`, `^`              |
| Comparison            | `==`, `!=`, `<`, `<=`, `>`, `>=` |
| Arithmetic            | `+`, `-`, unary `-`              |
| Shifting              | `shl`, `shr`                     |
| Rotation              | `rotl`, `rotr`                   |
| Bit manipulation      | `extract`, `concat`              |
| Conditional selection | `ite`                            |

All examples on this page assume a `z3::context ctx` is in scope.

## Logical

Logical operations for `SymBool`.

Unary operator typing rule:

| Operation | Operand     | Syntax | Result type |
| :-------- | :---------- | :----- | :---------- |
| NOT       | `SymBool a` | `!a`   | `SymBool`   |

Binary operator typing rules:

| Operation | LHS         | RHS         | Syntax     | Result type |
| :-------- | :---------- | :---------- | :--------- | :---------- |
| AND       | `SymBool a` | `SymBool b` | `a && b`   | `SymBool`   |
| OR        | `SymBool a` | `SymBool b` | `a \|\| b` | `SymBool`   |
| XOR       | `SymBool a` | `SymBool b` | `a ^ b`    | `SymBool`   |

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

| Operation | LHS            | RHS            | Syntax   | Result type  |
| :-------- | :------------- | :------------- | :------- | :----------- |
| AND       | `SymUInt<W> a` | `SymUInt<W> b` | `a & b`  | `SymUInt<W>` |
| AND       | `SymSInt<W> a` | `SymSInt<W> b` | `a & b`  | `SymSInt<W>` |
| OR        | `SymUInt<W> a` | `SymUInt<W> b` | `a \| b` | `SymUInt<W>` |
| OR        | `SymSInt<W> a` | `SymSInt<W> b` | `a \| b` | `SymSInt<W>` |
| XOR       | `SymUInt<W> a` | `SymUInt<W> b` | `a ^ b`  | `SymUInt<W>` |
| XOR       | `SymSInt<W> a` | `SymSInt<W> b` | `a ^ b`  | `SymSInt<W>` |

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

Comparison operations for `SymUInt` and `SymSInt`. Operand signednesses and bit
widths don't need to match. We are comparing the mathematical values the
operands represent, not their underlying bits.

Typing rules:

- Both LHS and RHS could be either `SymUInt` or `SymSInt`. Only one combination
  is shown below for brevity.

| Operation                | LHS            | RHS            | Syntax   | Result type |
| :----------------------- | :------------- | :------------- | :------- | :---------- |
| Equal to                 | `SymUInt<A> a` | `SymSInt<B> b` | `a == b` | `SymBool`   |
| Not equal to             | `SymUInt<A> a` | `SymSInt<B> b` | `a != b` | `SymBool`   |
| Greater than             | `SymUInt<A> a` | `SymSInt<B> b` | `a > b`  | `SymBool`   |
| Greater than or equal to | `SymUInt<A> a` | `SymSInt<B> b` | `a >= b` | `SymBool`   |
| Less than                | `SymUInt<A> a` | `SymSInt<B> b` | `a < b`  | `SymBool`   |
| Less than or equal to    | `SymUInt<A> a` | `SymSInt<B> b` | `a <= b` | `SymBool`   |

Examples:

```cpp
z3w::SymUInt<7> a(ctx, "a");
z3w::SymSInt<9> b(ctx, "b");

z3w::SymBool eq = (a == b);
z3w::SymBool ne = (a != b);
z3w::SymBool gt = (a > b);
z3w::SymBool ge = (a >= b);
z3w::SymBool lt = (a < b);
z3w::SymBool le = (a <= b);
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

Typing rules:

| Source           | High offset | Low offset | Syntax              | Result type      | Compile-time checks |
| :--------------- | :---------- | :--------- | :------------------ | :--------------- | :------------------ |
| `SymUInt<W> src` | `size_t H`  | `size_t L` | `extract<H,L>(src)` | `SymUInt<H-L+1>` | `W > H && H >= L`   |
| `SymSInt<W> src` | `size_t H`  | `size_t L` | `extract<H,L>(src)` | `SymUInt<H-L+1>` | `W > H && H >= L`   |

Examples:

```cpp
z3w::SymUInt<32> src(ctx, "src");

z3w::SymUInt<8> hi = z3w::extract<31, 24>(src);
z3w::SymUInt<4> lo = z3w::extract<3, 0>(src);
z3w::SymUInt<1> bit5 = z3w::extract<5, 5>(src);
```

### Symbolic-offset extraction

Typing rules:

| Source            | Target width | Bit offset      | Syntax                | Result type   | Compile-time checks |
| :---------------- | :----------- | :-------------- | :-------------------- | :------------ | :------------------ |
| `SymUInt<WS> src` | `size_t WT`  | `SymUInt<WI> i` | `extract<WT>(src, i)` | `SymUInt<WT>` | `WS >= WT`          |
| `SymSInt<WS> src` | `size_t WT`  | `SymUInt<WI> i` | `extract<WT>(src, i)` | `SymUInt<WT>` | `WS >= WT`          |

Examples:

```cpp
z3w::SymUInt<32> src(ctx, "src");
z3w::SymUInt<5> offset(ctx, "offset");

z3w::SymUInt<4> nibble = z3w::extract<4>(src, offset);
z3w::SymUInt<8> byte = z3w::extract<8>(src, offset);
```

Offsets beyond the source width yield zero bits (zero-extension semantics).

### Concatenation

Concatenate a variadic number of `SymUInt` or `SymSInt` inputs into a single
`SymUInt`. The field packing order is always from MSB to LSB.

Syntax: `concat(a, b, ...)`

Typing rules:

- Result type is always `SymUInt<W>`, where `W` is the sum of inputs' bit
  widths.

Examples:

```cpp
z3w::SymUInt<16> hi(ctx, "hi");
z3w::SymUInt<16> lo(ctx, "lo");

z3w::SymUInt<32> r1 = z3w::concat(hi, lo);

z3w::SymUInt<4> a(ctx, "a");
z3w::SymUInt<4> b(ctx, "b");
z3w::SymSInt<8> c(ctx, "c");
z3w::SymUInt<16> r2 = z3w::concat(a, b, c);
```

## Conditional selection

Symbolic if-then-else. Selects between two values based on a `SymBool`
condition.

Syntax: `ite(sel, a, b)`

Typing rules (choice types must be exactly the same):

| Condition     | Choice A       | Choice B       | Result type  |
| :------------ | :------------- | :------------- | :----------- |
| `SymBool sel` | `SymUInt<W> a` | `SymUInt<W> b` | `SymUInt<W>` |
| `SymBool sel` | `SymSInt<W> a` | `SymSInt<W> b` | `SymSInt<W>` |

Examples:

```cpp
z3w::SymBool sel(ctx, "sel");
z3w::SymUInt<8> a2(ctx, "a2");
z3w::SymUInt<8> b2(ctx, "b2");
z3w::SymSInt<8> a3(ctx, "a3");
z3w::SymSInt<8> b3(ctx, "b3");

z3w::SymUInt<8> r2 = z3w::ite(sel, a2, b2);
z3w::SymSInt<8> r3 = z3w::ite(sel, a3, b3);
```
