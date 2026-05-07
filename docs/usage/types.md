# Types

Z3Wire provides symbolic types that wrap Z3 expressions and concrete types that
store native values.

## Overview

| Symbolic (wraps `z3::expr`) | Concrete (stores native value) |
| :-------------------------- | :----------------------------- |
| `z3w::SymBool`              | `z3w::Bool`                    |
| `z3w::SymUInt<W>`           | `z3w::UInt<W>`                 |
| `z3w::SymSInt<W>`           | `z3w::SInt<W>`                 |

## Symbolic types

Symbolic types are thin wrappers around `z3::expr`. Bit-vector width must be at
least 1 (`SymUInt<0>` and `SymSInt<0>` are forbidden at compile time).

### Construction

Symbolic variables are constructed with a Z3 context and a symbol name:

```cpp
z3::context ctx;

z3w::SymBool flag(ctx, "flag");
z3w::SymUInt<8> x(ctx, "x");
z3w::SymSInt<8> y(ctx, "y");
```

`SymBool` literals:

```cpp
auto t = z3w::SymBool::True(ctx);
auto f = z3w::SymBool::False(ctx);
```

`SymUInt`/`SymSInt` literals are range-checked at compile time:

```cpp
auto a = z3w::SymUInt<8>::Literal<255>(ctx);  // OK
auto b = z3w::SymUInt<8>::Literal<256>(ctx);  // Compile error: value doesn't fit
auto c = z3w::SymSInt<8>::Literal<127>(ctx);  // OK
auto d = z3w::SymSInt<8>::Literal<128>(ctx);  // Compile error: value doesn't fit
```

### Z3 interop

Use `.expr()` to get the underlying `z3::expr` for interop with Z3 APIs:

```cpp
z3::solver solver(ctx);
z3w::SymUInt<8> x(ctx, "x");
solver.add(x.expr() > 0);  // Pass to Z3 solver directly
```

To wrap a raw `z3::expr` back into a Z3Wire type, use `From` (aborts on a
sort/width mismatch) or `TryFrom` (returns `std::optional`, nullopt on
mismatch):

```cpp
z3::expr raw = /* ... from Z3 API ... */;

// Aborts on mismatch.
auto x = z3w::SymUInt<8>::From(raw);
auto b = z3w::SymBool::From(raw);

// Non-aborting; returns std::nullopt on mismatch.
std::optional<z3w::SymUInt<8>> opt_x = z3w::SymUInt<8>::TryFrom(raw);
std::optional<z3w::SymBool> opt_b = z3w::SymBool::TryFrom(raw);
```

!!! note

    Z3Wire does not wrap `z3::context` or `z3::solver`. You interact with them
    directly.

## Concrete types

Concrete types store native values and do not require a `z3::context`.
Bit-vector width must be at least 1.

For widths up to 64, `.value()` returns a native integer type. Wide types (W >
64\) do not expose a byte-array `.value()` accessor; use `ToLeBytes()` or
`ToBeBytes()` to serialize to bytes explicitly (see "Byte serialization" below).

### Construction

`Bool` is constructed from `bool` (non-boolean types are rejected at compile
time):

```cpp
z3w::Bool flag = true;   // OK: implicit from bool
z3w::Bool flag2 = 42;    // Compile error: only bool accepted
```

`UInt`/`SInt` literals are range-checked at compile time:

```cpp
auto a = z3w::UInt<8>::Literal<255>();   // OK
auto b = z3w::UInt<8>::Literal<256>();   // Compile error: value doesn't fit
auto c = z3w::SInt<8>::Literal<127>();   // OK
auto d = z3w::SInt<8>::Literal<128>();   // Compile error: value doesn't fit
```

Runtime construction. Use `From` to abort on truncation, or `TryFrom` to
inspect:

```cpp
auto x = z3w::UInt<8>::From(200);
// x.value() == 200

auto r = z3w::UInt<8>::TryFrom(300);
// r.value.value() == 44, r.truncated == true (300 doesn't fit in 8 bits)
```

Construction from a byte sequence (any width). `FromLeBytes`/`FromBeBytes` and
`TryFromLeBytes`/`TryFromBeBytes` accept a `std::span<const uint8_t>`:

```cpp
std::array<uint8_t, 16> bytes = { /* ... */ };
auto w_le = z3w::UInt<128>::FromLeBytes(bytes);     // little-endian
auto w_be = z3w::UInt<128>::FromBeBytes(bytes);     // big-endian
auto [w, truncated] = z3w::UInt<128>::TryFromLeBytes(bytes);
```

Inputs of any length are accepted. Shorter than `(W + 7) / 8` bytes zero-pads on
the high-magnitude end (no truncation). Longer than that truncates and reports
it iff any extra bytes are non-zero. For W not a multiple of 8, the partial
byte's unused high bits must be zero or truncation is reported.

### Byte serialization

`ToLeBytes()` and `ToBeBytes()` return `std::array<uint8_t, (W + 7) / 8>`:

```cpp
auto x = z3w::UInt<32>::From(uint32_t{0xDEADBEEF});
x.ToLeBytes();  // {0xEF, 0xBE, 0xAD, 0xDE}
x.ToBeBytes();  // {0xDE, 0xAD, 0xBE, 0xEF}
```

For W not a multiple of 8, the value is right-aligned and padding lives in the
unused high bits of the most-significant byte (the partial byte at index
`kNumBytes - 1` for LE, index `0` for BE). For example, `UInt<12>` holding
`0xABC` serializes as `{0xBC, 0x0A}` LE or `{0x0A, 0xBC}` BE.

### Value access

`.value()` returns the stored value in its natural type (narrow types only):

```cpp
z3w::Bool flag = true;
flag.value();  // true

auto x = z3w::UInt<8>::Literal<200>();
x.value();  // 200 (uint8_t)

auto y = z3w::SInt<8>::Literal<-1>();
y.value();  // -1 (int8_t)
```

`Bool` also supports contextual conversion to `bool`:

```cpp
z3w::Bool flag = true;
if (flag) { /* ... */ }  // OK: contextual conversion to bool
```

## Type traits

Z3Wire provides type traits to distinguish its types in generic code:

```cpp
#include "z3wire/type_traits.h"

// Concrete types: Bool, UInt<W>, SInt<W>
static_assert(z3w::is_concrete_v<z3w::Bool>);
static_assert(z3w::is_concrete_v<z3w::UInt<8>>);
static_assert(!z3w::is_concrete_v<int>);

// Symbolic types: SymBool, SymUInt<W>, SymSInt<W>
static_assert(z3w::is_symbolic_v<z3w::SymBool>);
static_assert(z3w::is_symbolic_v<z3w::SymUInt<8>>);
static_assert(!z3w::is_symbolic_v<int>);
```
