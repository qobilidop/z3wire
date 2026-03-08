# Operations

## Arithmetic (bit-growth)

To prevent silent overflow, addition and subtraction automatically widen the
result type. This is inspired by
[CIRCT HWArith](https://circt.llvm.org/docs/Dialects/HWArith/RationaleHWArith/).

### Addition (`+`)

Result width = `max(W1, W2) + 1`. Result is signed if either operand is signed.

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> b(ctx, "b");
auto sum = a + b;       // z3w::Ubv<9>
auto total = sum + a;   // z3w::Ubv<10>
```

Operands of different widths are automatically extended:

```cpp
z3w::Ubv<8> x(ctx, "x");
z3w::Ubv<16> y(ctx, "y");
auto sum = x + y;  // z3w::Ubv<17>, x is zero-extended to 16 bits first
```

### Subtraction (`-`)

Result width = `max(W1, W2) + 1`. Result is always signed (subtraction can
produce negative values).

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> b(ctx, "b");
auto diff = a - b;  // z3w::Sbv<9>
```

!!! tip
    To model hardware-style fixed-width arithmetic, use `cast` to explicitly
    truncate the result:

    ```cpp
    auto reg = z3w::cast<z3w::Ubv<8>>(a + b);  // Truncate to 8 bits
    ```

## Bitwise operations (strict)

Bitwise operators require operands of the **exact same width and signedness**.
Mismatches are compile errors.

| Operator | Description |
|:---------|:------------|
| `a & b` | Bitwise AND |
| `a \| b` | Bitwise OR |
| `a ^ b` | Bitwise XOR |
| `~a` | Bitwise NOT |

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> b(ctx, "b");
auto masked = a & b;  // z3w::Ubv<8>
auto flipped = ~a;    // z3w::Ubv<8>
```

## Equality (strict)

Requires the same width and signedness. Returns `z3w::Bool`.

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> b(ctx, "b");
z3w::Bool eq = (a == b);
z3w::Bool ne = (a != b);
```

## Ordered comparison (strict)

Requires the same width and signedness. Automatically dispatches to the correct
Z3 function based on signedness. Returns `z3w::Bool`.

| Operator | Unsigned | Signed |
|:---------|:---------|:-------|
| `a < b` | `bvult` | `bvslt` |
| `a <= b` | `bvule` | `bvsle` |
| `a > b` | `bvugt` | `bvsgt` |
| `a >= b` | `bvuge` | `bvsge` |

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> b(ctx, "b");
z3w::Bool less = (a < b);  // Uses bvult (unsigned)

z3w::Sbv<8> x(ctx, "x");
z3w::Sbv<8> y(ctx, "y");
z3w::Bool less_s = (x < y);  // Uses bvslt (signed)
```

## Bool operations

`z3w::Bool` supports standard logical operations:

```cpp
z3w::Bool a(ctx, "a");
z3w::Bool b(ctx, "b");

z3w::Bool c = a && b;   // Logical AND
z3w::Bool d = a || b;   // Logical OR
z3w::Bool e = !a;       // Logical NOT
```

## Conditional selection (`ite`)

Symbolic If-Then-Else. Both branches must be the exact same type.

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> b(ctx, "b");
z3w::Bool sel(ctx, "sel");

auto result = z3w::ite(sel, a, b);  // z3w::Ubv<8>
```
