# Concrete Types

Why Z3Wire has concrete types (`Bool`, `UInt<W>`, `SInt<W>`), why they have
minimal operations, and what role they play in the type system.

## The decision

Z3Wire provides concrete types parallel to its symbolic types:

| Concrete  | Symbolic     | Value set                    |
| --------- | ------------ | ---------------------------- |
| `Bool`    | `SymBool`    | {false, true}                |
| `UInt<W>` | `SymUInt<W>` | {0, ..., 2^W − 1}            |
| `SInt<W>` | `SymSInt<W>` | {−2^(W−1), ..., 2^(W−1) − 1} |

Concrete types hold values with compile-time width and signedness. They have
almost no operations — no arithmetic, no bitwise, no comparisons beyond
equality. Their purpose is to carry typed values, not to compute.

## Why have concrete types at all

### Compile-time width and signedness

Native C++ integers have platform-fixed widths (`int` is 32 bits, `long long` is
64 bits). Z3Wire needs arbitrary widths — 7 bits, 128 bits, any `W >= 1`.
Concrete types encode this in the type system:

```cpp
z3w::UInt<7> a;    // 7-bit unsigned — no native C++ equivalent
z3w::UInt<128> b;  // 128-bit unsigned — beyond native integer range
z3w::SInt<3> c;    // 3-bit signed — range [-4, 3]
```

### Compile-time range checking

`Literal<V>()` verifies at compile time that the value fits the type:

```cpp
auto ok  = z3w::UInt<8>::Literal<255>();  // compiles
auto bad = z3w::UInt<8>::Literal<256>();  // compile error
auto neg = z3w::SInt<8>::Literal<-128>(); // compiles
auto too = z3w::SInt<8>::Literal<-129>(); // compile error
```

Without concrete types, symbolic constructors would need to accept raw integers,
deferring range checking to runtime or skipping it entirely.

### Guarding against native C++ implicit conversions

C++ implicit conversions between native types are a well-known source of bugs:
`int` silently converts to `bool`, `double` narrows to `int`, negative values
wrap to large unsigned values. Concrete types act as a firewall:

- **`Bool`** — only accepts `bool`. The constructor
    `template <typename T>   Bool(T) = delete` explicitly rejects `int`,
    `double`, pointers, and everything else. In native C++, `bool b = 42`
    compiles silently.
- **`BitVec`** — `Literal<V>()` range-checks at compile time. `FromValue(raw)`
    returns a truncation flag at runtime. Neither silently narrows.
- **No implicit widening** — `UInt<8>` doesn't implicitly convert to `UInt<16>`.
    Widening requires `safe_cast` or an explicit mixed operation.

This isn't perfect — the protection exists at Z3Wire's API boundary, not within
native C++ code. But at the boundary where values enter the Z3Wire type system,
the concrete types catch the most common mistakes.

## Why minimal operations

### Computation belongs elsewhere

There are two domains where computation makes sense:

1. **Symbolic** — Z3 reasons about symbolic expressions. This is the core of
    Z3Wire.
1. **Native C++** — the CPU evaluates concrete values. Standard C++ handles
    this.

Concrete types adding arithmetic would create a third computation domain that
duplicates native C++ functionality without adding value. `UInt<8>` arithmetic
would just be `uint8_t` arithmetic with extra steps.

### The data holder role

Concrete types are **typed data holders**, not computation types. Their API
reflects this:

| What they do        | API                              |
| ------------------- | -------------------------------- |
| Hold a typed value  | `Literal<V>()`, `FromValue(raw)` |
| Report their value  | `value()`                        |
| Check equality      | `operator==`                     |
| Promote to symbolic | `to_symbolic(val, ctx)`          |
| Print               | `operator<<`                     |

This is deliberately minimal. Each concrete type is essentially a compile-time
checked wrapper around a native integer (or byte array for W > 64).

### Auto-promotion makes operations unnecessary

In mixed expressions with symbolic types, concrete values auto-promote:

```cpp
z3w::SymUInt<8> x(ctx, "x");
auto lit = z3w::UInt<8>::Literal<42>();

auto sum = x + lit;          // UInt<8> promotes to SymUInt<8>, then bit-growth
z3w::SymBool eq = (x == lit); // UInt<8> promotes, then mathematical comparison
```

The programmer never needs concrete-on-concrete operations because the
interesting work happens after promotion to the symbolic domain.

## Connection to other design decisions

- **[Lossless Auto-Promotion](lossless-auto-promotion.md)** — concrete types are
    the middle tier (native → concrete → symbolic). Promotion from concrete to
    symbolic is always lossless and happens automatically in mixed expressions.
- **[Three-Tier Casting](three-tier-casting.md)** — `safe_cast`, `checked_cast`,
    and `unsafe_cast` apply to both concrete and symbolic types, maintaining the
    same safety guarantees across tiers.
- **[Bool vs UInt\<1>](bool-vs-uint1.md)** — the concrete/symbolic split applies
    to both booleans and bit-vectors, with the same explicit conversion
    boundary.
