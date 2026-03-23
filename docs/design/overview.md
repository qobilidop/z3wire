# Design Overview

## Motivation

Z3's C++ API represents all expressions as `z3::expr`, with no compile-time
distinction between Booleans and bit-vectors, no width tracking, and no
signedness information. This means:

- Adding a 32-bit vector to an 8-bit vector compiles silently but crashes at
  runtime with `Z3_SORT_ERROR`.
- Comparing bit-vectors requires choosing the right function (`z3::ult` vs
  `z3::slt` for unsigned vs signed less-than), but nothing prevents calling the
  wrong one.
- A Bool passed where a bit-vector is expected is only caught when Z3 evaluates
  the expression.

Furthermore, Z3's arithmetic operates at fixed widths — adding two 8-bit vectors
produces an 8-bit result, silently discarding the carry bit. In hardware
modeling, this silent overflow is a major source of subtle verification bugs: a
proof may pass not because the design is correct, but because the formula itself
lost information. Catching overflow requires manually widening operands before
every arithmetic operation, which is tedious and error-prone.

Z3Wire addresses both problems: compile-time type safety eliminates sort errors,
and bit-growth arithmetic makes overflow explicit.

## Core goals

1. **Compile-time type safety:** Move Z3 "Sort Errors" (bit-width/type
   mismatches) from runtime exceptions to compile-time errors using C++20
   template metaprogramming.
1. **Bit-growth arithmetic:** Automatically widen arithmetic result types to
   prevent silent overflow, making every truncation an explicit, reviewable
   decision.

## Scope

Z3 supports many SMT theories — unbounded integers, reals, arrays,
floating-point, uninterpreted functions — but Z3Wire intentionally covers only
**Booleans** and **fixed-width bit-vectors**.

The reason is in the name: hardware is built from _wires_. Every signal in a
digital circuit is either a single bit (boolean) or a bundle of bits with a
known width (bit-vector). Unbounded integers, reals, and other abstract
mathematical types do not correspond to physical hardware and are irrelevant to
RTL modeling and verification.

By targeting only the QF_BV logic (Quantifier-Free Bit-Vectors), Z3Wire can:

- Provide a tight, ergonomic API with no unused abstractions.
- Enforce hardware-meaningful constraints (fixed widths, explicit truncation).
- Keep the library small and auditable.

**Out of scope for this project:**

- Non-hardware SMT theories (unbounded integers, reals, arrays, floating-point,
  uninterpreted functions).
- Wrapping `z3::context` or `z3::solver`.

## Combinational logic primitives

Z3Wire's API is organized around the complete set of combinational logic
primitives — the operations a hardware designer uses to build circuits from
wires. The table below lists every category, its operations, and their status.

| Category         | Operations                          | Status                    |
| :--------------- | :---------------------------------- | :------------------------ |
| Logic (SymBool)  | AND, OR, NOT, equality              | Done                      |
| Logic (SymBool)  | XOR                                 | Gap (expressible as `!=`) |
| Bitwise          | AND, OR, XOR, NOT                   | Done                      |
| Arithmetic       | Add, subtract, negate               | Done                      |
| Comparison       | Equality, ordered                   | Done                      |
| Shifting         | Left shift (shl), right shift (shr) | Done                      |
| Bit manipulation | Extract, replace, concat            | Done                      |
| Bit field        | Field equality constraint           | Done                      |
| Mux              | If-then-else                        | Done                      |
| Type conversion  | Cast (3-tier), SymBool/SymUInt\<1>  | Done                      |
| Reduction        | AND, OR, XOR across all bits        | Future                    |

This framing guides what belongs in Z3Wire and what does not. If an operation is
a standard combinational logic primitive, it should be here. If it requires
sequential semantics (clocks, state, memory), it is out of scope.

## Design philosophy

- **Zero overhead:** Each wrapper stores only a `z3::expr`. No virtual
  functions, no extra data members.
- **Explicit over implicit:** No implicit conversions between signed/unsigned or
  different widths. Users must use the casting API to express intent.
- **Header-based template library.** The core types and operators are templates
  and must live in headers. Any non-template utilities should go in `.cc` files
  per Google C++ style guide conventions.

## Type system

The library centers around a zero-overhead wrapper class that holds a single
`z3::expr` by value, adding no runtime overhead.

All symbolic types use a `Sym` prefix to clearly distinguish them from concrete
types. Concrete types use natural C++ names.

| Symbolic              | Concrete           | Description                      |
| :-------------------- | :----------------- | :------------------------------- |
| `z3w::SymUInt<W>`     | `z3w::UInt<W>`     | Unsigned fixed-width bit-vector. |
| `z3w::SymSInt<W>`     | `z3w::SInt<W>`     | Signed fixed-width bit-vector.   |
| `z3w::SymBool`        | `z3w::Bool`        | Boolean.                         |
| `z3w::SymBitVec<W,S>` | `z3w::BitVec<W,S>` | Underlying template.             |

The core template is:

```cpp
template <size_t Width, bool IsSigned>
class SymBitVec {
    static_assert(Width > 0, "Bit-vector width must be at least 1.");
    z3::expr expr_;
    // ...
};

template <size_t W> using SymUInt = SymBitVec<W, false>;
template <size_t W> using SymSInt = SymBitVec<W, true>;
```

Note: `SymBitVec<0, S>` is forbidden via `static_assert`. A zero-width
bit-vector has no meaning in hardware or SMT.

## Construction and literals

### Compile-time range-checked literals

```cpp
template <uint64_t Value>
static SymBitVec<Width, IsSigned> Literal(z3::context& ctx);
```

Uses `static_assert` to verify the value fits in the specified width at compile
time.

```cpp
auto ok  = z3w::SymUInt<8>::Literal<255>(ctx);  // Compiles
auto bad = z3w::SymUInt<8>::Literal<256>(ctx);  // Compile error!
```

### Symbolic variables

```cpp
// Constructor taking a name creates a symbolic variable
z3w::SymUInt<32> x(ctx, "x");
```

## Type conversions

### Three-tier casting API

#### `z3w::unsafe_cast<T>(val)` - The hardware cast

Performs raw truncation, extension, or sign reinterpretation based on source and
target types. No safety checks. Use when you intentionally want hardware-style
overflow or wrap behavior.

Under the hood, uses `if constexpr` to select:

- Target width < source width: `z3::extract` (truncation).
- Target width > source width: `z3::zext` or `z3::sext` based on source
  signedness.
- Target width == source width: zero-overhead type reinterpretation (bitcast).

#### `z3w::safe_cast<T>(val)` —The compiler guard

Only compiles if the cast is mathematically guaranteed to be lossless.

| Source        | Target        | Allowed?                                        |
| :------------ | :------------ | :---------------------------------------------- |
| `SymUInt<W1>` | `SymUInt<W2>` | Yes, if `W2 >= W1`.                             |
| `SymSInt<W1>` | `SymSInt<W2>` | Yes, if `W2 >= W1`.                             |
| `SymUInt<W1>` | `SymSInt<W2>` | Yes, if `W2 > W1` (needs 1 extra bit for sign). |
| `SymSInt<W1>` | `SymUInt<W2>` | **Always forbidden.** Negative values corrupt.  |
| Any           | Smaller       | **Always forbidden.** Truncation is not safe.   |

#### `z3w::checked_cast<T>(val)` —The verification cast

Returns `std::tuple<T, z3w::SymBool>`. Performs the cast and also returns a
symbolic boolean formula representing whether the mathematical value was
preserved. The user can assert the boolean into the solver to verify safety.

```cpp
auto [result, value_preserved] = z3w::checked_cast<z3w::SymUInt<8>>(my_32bit_val);
solver.add(value_preserved.expr());  // Assert: this cast never loses data
```

### SymBool / SymUInt\<1> conversion

In Z3, `SymBool` and a 1-bit bit-vector are distinct sorts. Hardware frequently
needs to convert between them (e.g., a condition flag in a register vs. a
logical condition). Z3Wire provides explicit conversion functions:

- **`z3w::as_bool(SymUInt<1>)`** — converts a 1-bit vector to SymBool (true if
  bit is 1).
- **`z3w::as_uint1(SymBool)`** — converts a SymBool to `SymUInt<1>` (1 if true,
  0 if false).

```cpp
z3w::SymUInt<32> status(ctx, "status");
z3w::SymBool ready = z3w::as_bool(z3w::extract<0, 0>(status));

z3w::SymBool cond(ctx, "cond");
z3w::SymUInt<1> flag = z3w::as_uint1(cond);
```

## Bit manipulation

### Bit slicing (`extract`)

#### Fully static (compile-time bounds)

```cpp
template <size_t High, size_t Low, size_t InWidth, bool S>
auto extract(const SymBitVec<InWidth, S>& val);
// Returns z3w::SymUInt<High - Low + 1>
```

Example: `auto opcode = z3w::extract<31, 24>(instruction);`

#### Symbolic offset + static width

```cpp
template <size_t TargetWidth, size_t InWidth, bool S, size_t IdxWidth>
z3w::SymUInt<TargetWidth> extract(const SymBitVec<InWidth, S>& val,
                               const z3w::SymUInt<IdxWidth>& start_idx);
```

Implemented via barrel-shifting: shift right by the symbolic offset, then
statically extract the bottom `TargetWidth` bits.

Example: `auto nibble = z3w::extract<4>(packet, symbolic_offset);`

### Concatenation (`concat`)

Glues bit-vectors together. The result width is `W1 + W2`, always returned as
`SymUInt` (raw bits have no inherent signedness). Supports variadic arguments.

```cpp
z3w::SymUInt<16> high(ctx, "high");
z3w::SymUInt<16> low(ctx, "low");
auto full = z3w::concat(high, low);  // -> z3w::SymUInt<32>
```

## Operations

This section covers design decisions for each category of
[combinational logic primitives](#combinational-logic-primitives).

### Arithmetic and bitwise

To prevent silent overflows, Z3Wire adopts bit-growth semantics for arithmetic,
inspired by
[CIRCT HWArith](https://circt.llvm.org/docs/Dialects/HWArith/RationaleHWArith/).
Bitwise and shift operations enforce strict width and signedness matching.
Arithmetic and comparison operations allow mixed widths and signedness,
automatically extending operands to a common type.

- **Addition / Subtraction (`+`, `-`):** Result width is `max(W1, W2) + 1`.
  Operands are automatically sign-extended or zero-extended to match before the
  operation. (Bit growth.)
- **Bitwise logic (`&`, `|`, `^`):** Widths and signedness must match exactly.
  (Strict.)
- **Equality (`==`, `!=`):** Allows different widths and signedness. Operands
  are extended to a common type before comparing. (Relaxed.)
- **Ordered comparison (`<`, `<=`, `>`, `>=`):** Allows different widths and
  signedness. Operands are extended to a common type. Signedness-aware:
  dispatches to unsigned or signed comparison based on the common type (signed
  if either operand is signed). Returns `z3w::SymBool`. (Relaxed.)

Example:

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
auto sum = a + b;       // -> z3w::SymUInt<9>
auto total = sum + a;   // -> z3w::SymUInt<10>

// To model hardware overflow, explicitly truncate:
auto reg = z3w::unsafe_cast<z3w::SymUInt<8>>(total);
```

### SymBool operations

`z3w::SymBool` supports standard logical operations:

- **Logical:** `&&`, `||`, `!`
- **Literals:** `z3w::SymBool::True(ctx)`, `z3w::SymBool::False(ctx)`

```cpp
z3w::SymBool a(ctx, "a");
z3w::SymBool b(ctx, "b");

z3w::SymBool c = a && b;
z3w::SymBool d = !a || b;
z3w::SymBool t = z3w::SymBool::True(ctx);
```

### Shifting

Z3Wire provides two shift primitives: `shl` (left shift) and `shr` (right
shift). Left shifts always widen the result to guarantee no bits are lost. Other
shift patterns are achieved through composability with casting and signedness
reinterpretation.

#### `shl` - Left shift

`shl` always returns `SymUInt` regardless of input signedness. The result widens
to prevent bit loss.

- **Constant shift `N`:** result width = `W + N`.
- **Symbolic shift `SymUInt<K>`:** result width = `W + 2^K - 1` (assumes the
  maximum representable shift amount).

```cpp
z3w::SymUInt<8> a(ctx, "a");

// Compile-time constant shift
auto r1 = z3w::shl<3>(a);    // -> z3w::SymUInt<11>

// Symbolic shift
z3w::SymUInt<3> n(ctx, "n");
auto r2 = z3w::shl(a, n);    // -> z3w::SymUInt<15>
```

#### `shr` - Right shift

Arithmetic right shift. Preserves sign for signed types (`ashr`), zero-fills for
unsigned types (`lshr`). Result width stays the same.

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<3> n(ctx, "n");
auto r = z3w::shr(a, n);  // -> z3w::SymUInt<8>
```

#### Composability

Rather than providing multiple shift tiers, `shl`/`shr` compose with the
existing casting and signedness APIs:

| Goal                          | Expression                                |
| :---------------------------- | :---------------------------------------- |
| Raw left shift (fixed width)  | `unsafe_cast<SymUInt<W>>(shl<N>(val))`    |
| Checked left shift (verify)   | `checked_cast<SymUInt<W>>(shl<N>(val))`   |
| Logical right shift on signed | `shr(as_unsigned(val), as_unsigned(amt))` |

### Conditional selection (`ite`)

Symbolic If-Then-Else. Works for any Z3Wire symbolic type (`SymBool`,
`SymUInt<W>`, `SymSInt<W>`). Both branches must be the exact same type.

```cpp
template <typename T>
T ite(const z3w::SymBool& cond, const T& true_val, const T& false_val);
```

```cpp
z3w::SymUInt<8> a(ctx, "a");
z3w::SymUInt<8> b(ctx, "b");
z3w::SymBool sel(ctx, "sel");

auto result = z3w::ite(sel, a, b);  // -> z3w::SymUInt<8>
auto flag = z3w::ite(sel, z3w::SymBool::True(ctx), z3w::SymBool::False(ctx));
```

## Concrete types

### Motivation

Concrete types (`UInt<W>`, `SInt<W>`) serve as type-safe value holders:

1. **Mixed expressions.** Users can write `symbolic_x + concrete_y` without
   manually creating Z3 constants. The concrete value auto-promotes.
1. **Type-safe storage.** `UInt<5>` or `SInt<12>` enforce bit-width at the type
   level, even without Z3.

A concrete `Bool` type wraps native `bool` with type-safe construction. Its
integral constructor is deleted, so `Bool b = 42;` is a compile error while
`Bool b = true;` works naturally. `explicit operator bool()` prevents accidental
use in arithmetic but allows contextual conversion (`if (b)`). Mixed
`Bool`/`SymBool` operands are not auto-promoted — concrete `Bool` carries no Z3
context, so promotion requires an explicit `to_symbolic(Bool, z3::context&)`
call.

### Storage

`UInt<W>` uses the smallest unsigned integer type that fits W bits, and
`SInt<W>` uses the smallest signed integer type. Width is constrained to 1-64.

### Promotion to symbolic

Concrete values implicitly promote to symbolic when mixed in expressions. This
is the one exception to "explicit over implicit": the promotion is always
lossless and unambiguous (same width, same signedness), so it is safe to allow
implicitly. The Z3 context is grabbed from the symbolic operand's `z3::expr`.

### Operations

Concrete types support construction (raw, `Literal`, `Checked`), value access
(`.value()`), and equality (`==`, `!=`). All other operations (arithmetic,
bitwise, shifts, casting, etc.) are available only on symbolic types. Concrete
values auto-promote to symbolic in mixed expressions, so users can write
`sym + UInt<8>(5)` without manual conversion.

## Boundary layer

- **`val.expr()`**: Returns the underlying `z3::expr` for interop with raw Z3
  APIs (e.g., passing to `z3::solver`).
- Users interact with `z3::context` and `z3::solver` directly; Z3Wire does not
  wrap these.
