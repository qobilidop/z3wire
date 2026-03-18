# Operations

Z3Wire operations are organized by combinational logic category. All operations
work on symbolic types (`SymBool`, `SymUInt<W>`, `SymSInt<W>`). Concrete types
(`Bool`, `UInt<W>`, `SInt<W>`) support only equality (`==`, `!=`); for all other
operations, use symbolic types or access `.value()` for native C++ arithmetic.
Concrete values auto-promote to symbolic in mixed expressions.

## Logic

`z3w::SymBool` supports standard logical operations:

```cpp
z3w::SymBool a(ctx, "a");
z3w::SymBool b(ctx, "b");

z3w::SymBool c = a && b;   // Logical AND
z3w::SymBool d = a || b;   // Logical OR
z3w::SymBool e = !a;       // Logical NOT
```

## Bitwise

Bitwise operators require operands of the **exact same width and signedness**.
Mismatches are compile errors.

| Operator | Description |
| :------- | :---------- |
| `a & b`  | Bitwise AND |
| `a \| b` | Bitwise OR  |
| `a ^ b`  | Bitwise XOR |
| `~a`     | Bitwise NOT |

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
auto masked = a & b;  // z3w::SymUInt<8>
auto flipped = ~a;    // z3w::SymUInt<8>
```

## Arithmetic

To prevent silent overflow, addition and subtraction automatically widen the
result type. This is inspired by
[CIRCT HWArith](https://circt.llvm.org/docs/Dialects/HWArith/RationaleHWArith/).
Mixed widths and signedness are allowed; operands are automatically extended.

### Addition (`+`)

Result width = `max(W1, W2) + 1`. Result is signed if either operand is signed.

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
auto sum = a + b;       // z3w::SymUInt<9>
auto total = sum + a;   // z3w::SymUInt<10>

// Different widths:
z3w::SymUInt<16> y(ctx, "y");
auto sum2 = a + y;  // z3w::SymUInt<17>, a is zero-extended first
```

### Subtraction (`-`)

Result width = `max(W1, W2) + 1`. Result is always signed (subtraction can
produce negative values).

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
auto diff = a - b;  // z3w::SymSInt<9>
```

### Unary negate (`-`)

Result width = `W + 1`. Result is always signed. Consistent with subtraction
(`-x` is equivalent to `0 - x`).

```cpp
z3w::SymUInt<8> a(ctx, "a");
auto neg = -a;  // z3w::SymSInt<9>
```

!!! tip

    To model hardware-style fixed-width arithmetic, use `unsafe_cast` to explicitly
    truncate the result:

    ```cpp
    auto reg = z3w::unsafe_cast<z3w::SymUInt<8>>(a + b);  // Truncate to 8 bits
    ```

## Comparison

### Equality (relaxed)

Allows different widths and signedness. Operands are extended to a common type
before comparing. Returns `z3w::SymBool`.

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
z3w::SymBool eq = (a == b);
z3w::SymBool ne = (a != b);

// Different widths:
z3w::SymUInt<16> c(ctx, "c");
z3w::SymBool eq2 = (a == c);  // a is zero-extended to 16 bits first
```

### Ordered comparison (relaxed)

Allows different widths and signedness. Automatically dispatches to unsigned or
signed comparison based on the common type (signed if either operand is signed).
Returns `z3w::SymBool`.

| Operator | Unsigned | Signed  |
| :------- | :------- | :------ |
| `a < b`  | `bvult`  | `bvslt` |
| `a <= b` | `bvule`  | `bvsle` |
| `a > b`  | `bvugt`  | `bvsgt` |
| `a >= b` | `bvuge`  | `bvsge` |

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
z3w::SymBool less = (a < b);  // Uses bvult (unsigned)

z3w::SymSInt<8> x(ctx, "x");
z3w::SymSInt<8> y(ctx, "y");
z3w::SymBool less_s = (x < y);  // Uses bvslt (signed)

// Mixed signedness — extends both to a common signed type:
z3w::SymBool less_m = (a < x);  // Common type: SymSInt<9>
```

## Shifting

Z3Wire provides two shift functions - `shl` (left shift) and `shr` (right
shift). Left shifts always widen the result to guarantee no bits are lost.

### `shl` - Left shift

Left shift always returns `SymUInt` regardless of input signedness.

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

Arithmetic right shift. Preserves sign for signed types (`ashr`), zero-fills for
unsigned types (`lshr`). Result width stays the same.

**Constant shift**:

```cpp
z3w::SymUInt<8> a(ctx, "a");
auto r = z3w::shr<3>(a);  // SymUInt<8>, zero-filled from the left
```

**Symbolic shift** - shift amount width may differ from value width:

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<3> n(ctx, "n");
auto r = z3w::shr(a, n);  // SymUInt<8>, zero-filled from the left

z3w::SymSInt<8> s(ctx, "s");
z3w::SymUInt<3> m(ctx, "m");
auto r2 = z3w::shr(s, m);  // SymSInt<8>, sign bit preserved
```

### Composability

The `shl`/`shr` primitives compose with casting and signedness reinterpretation
to cover all common shift patterns:

| Goal                          | Expression                              |
| :---------------------------- | :-------------------------------------- |
| Raw left shift (fixed width)  | `unsafe_cast<SymUInt<W>>(shl<N>(val))`  |
| Checked left shift (verify)   | `checked_cast<SymUInt<W>>(shl<N>(val))` |
| Logical right shift on signed | `shr(as_unsigned(val), amt)`            |

## Bit manipulation

### Single-bit extraction

Shorthand for extracting a single bit:

```cpp
z3w::SymUInt<32> instruction(ctx, "instruction");

auto sign = z3w::bit<31>(instruction);  // SymUInt<1>
auto lsb  = z3w::bit<0>(instruction);   // SymUInt<1>
```

Equivalent to `z3w::extract<N, N>(val)`.

### Extract (bit slicing)

**Static extract** — compile-time bounds, checked at compile time (`High >= Low`
and `High < W`):

```cpp
z3w::SymUInt<32> instruction(ctx, "instruction");

auto opcode = z3w::extract<31, 24>(instruction);  // SymUInt<8>
auto reg = z3w::extract<3, 0>(instruction);        // SymUInt<4>
auto bit5 = z3w::extract<5, 5>(instruction);       // SymUInt<1>
```

!!! note

    The result is always `SymUInt` (unsigned). Extracted bits have no inherent
    signedness.

**Symbolic-offset extract** — fixed number of bits starting at a symbolic
position:

```cpp
z3w::SymUInt<32> data(ctx, "data");
z3w::SymUInt<5> offset(ctx, "offset");

auto nibble = z3w::extract<4>(data, offset);  // SymUInt<4>
auto byte = z3w::extract<8>(data, offset);    // SymUInt<8>
```

### Concatenation

Glue bit-vectors together. The result width is `W1 + W2`, always returned as
`SymUInt`.

```cpp
z3w::SymUInt<16> high(ctx, "high");
z3w::SymUInt<16> low(ctx, "low");
auto full = z3w::concat(high, low);  // SymUInt<32>
```

Supports variadic arguments:

```cpp
z3w::SymUInt<4> a(ctx, "a");
z3w::SymUInt<4> b(ctx, "b");
z3w::SymUInt<8> c(ctx, "c");
auto packed = z3w::concat(a, b, c);  // SymUInt<16>
```

!!! tip

    `concat` and `extract` are complementary. A common hardware pattern is unpacking
    a word into fields, modifying a field, and repacking:

    ```cpp
    z3w::SymUInt<16> word(ctx, "word");
    auto hi = z3w::extract<15, 8>(word);  // SymUInt<8>
    auto lo = z3w::extract<7, 0>(word);   // SymUInt<8>
    auto repacked = z3w::concat(hi, lo);  // SymUInt<16>
    ```

## Mux

Symbolic If-Then-Else. Both branches must be the exact same type.

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
z3w::SymBool sel(ctx, "sel");

auto result = z3w::ite(sel, a, b);  // z3w::SymUInt<8>
```

Works with concrete values in mixed expressions (they auto-promote to symbolic):

```cpp
z3w::SymBool cond(ctx, "c");
z3w::UInt<8> a(42);
z3w::UInt<8> b(99);

auto result = z3w::ite(cond, a, b);  // Promotes both to symbolic
```
