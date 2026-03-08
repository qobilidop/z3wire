# Type System

Z3Wire provides three core types, all zero-overhead wrappers around `z3::expr`.

## Types

| Type | Description |
|:-----|:------------|
| `z3w::Bool` | Symbolic boolean |
| `z3w::Ubv<W>` | Unsigned bit-vector of width `W` |
| `z3w::Sbv<W>` | Signed bit-vector of width `W` |

`Ubv<W>` and `Sbv<W>` are aliases for the underlying template:

```cpp
template <size_t Width, bool IsSigned>
class BitVec;

template <size_t W> using Ubv = BitVec<W, false>;
template <size_t W> using Sbv = BitVec<W, true>;
```

!!! info
    `BitVec<0, S>` is forbidden via `static_assert`. A zero-width bit-vector has
    no meaning in hardware or SMT.

## Creating Symbolic Variables

All types are constructed with a Z3 context and a name:

```cpp
z3::context ctx;

z3w::Bool flag(ctx, "flag");
z3w::Ubv<32> address(ctx, "address");
z3w::Sbv<16> offset(ctx, "offset");
```

## Literals

### Bool Literals

```cpp
z3w::Bool t = z3w::Bool::True(ctx);
z3w::Bool f = z3w::Bool::False(ctx);
```

### Bit-Vector Literals

Use the `Literal` static method with a compile-time value. The value is
range-checked at compile time:

```cpp
auto x = z3w::Ubv<8>::Literal<255>(ctx);  // OK
auto y = z3w::Ubv<8>::Literal<256>(ctx);  // Compile error: value doesn't fit
```

## Accessing the Underlying Z3 Expression

Use `.raw()` to get the underlying `z3::expr` for interop with Z3 APIs:

```cpp
z3w::Ubv<8> x(ctx, "x");
solver.add(x.raw() > 0);  // Pass to Z3 solver directly
```

!!! note
    Z3Wire does not wrap `z3::context` or `z3::solver`. You interact with these
    directly.

## Bool / Ubv<1> Conversion

In Z3, `Bool` and a 1-bit bit-vector are distinct sorts. Hardware frequently
needs to convert between them. Z3Wire provides explicit conversion functions:

```cpp
// Bit-vector to Bool
z3w::Ubv<32> status(ctx, "status");
z3w::Bool ready = z3w::to_bool(z3w::extract<0, 0>(status));

// Bool to bit-vector
z3w::Bool cond(ctx, "cond");
z3w::Ubv<1> flag = z3w::to_ubv1(cond);
```

## Design Principle: Explicit Over Implicit

Z3Wire never implicitly converts between types. There are no implicit
conversions between:

- `Bool` and `Ubv<1>`
- Signed and unsigned bit-vectors
- Bit-vectors of different widths

All conversions must go through the [casting API](casting.md).
