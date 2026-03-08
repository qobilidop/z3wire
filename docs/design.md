# Z3Wire Design Document

## Project Identity

- **Name:** Z3Wire
- **Namespace:** `z3w::`
- **One-liner:** A type-safe C++20 template library for Z3 that enforces
  compile-time bit-width consistency and bit-growth arithmetic for hardware
  modeling and formal verification.
- **Target audience:** Hardware designers, verification engineers, and security
  researchers.

## Motivation

Z3's C++ API represents all expressions as `z3::expr`, with no compile-time
distinction between Booleans and bit-vectors, no width tracking, and no
signedness information. This means:

- Adding a 32-bit vector to an 8-bit vector compiles silently but crashes at
  runtime with `Z3_SORT_ERROR`.
- Comparing bit-vectors requires choosing the right function (`z3::ult` vs
  `z3::slt` for unsigned vs signed less-than), but nothing prevents calling
  the wrong one.
- A Bool passed where a bit-vector is expected is only caught when Z3 evaluates
  the expression.

Furthermore, Z3's arithmetic operates at fixed widths — adding two 8-bit vectors
produces an 8-bit result, silently discarding the carry bit. In hardware
modeling, this silent overflow is a major source of subtle verification bugs:
a proof may pass not because the design is correct, but because the formula
itself lost information. Catching overflow requires manually widening operands
before every arithmetic operation, which is tedious and error-prone.

Z3Wire addresses both problems: compile-time type safety eliminates sort errors,
and bit-growth arithmetic makes overflow explicit.

## Core Goals

1. **Compile-time type safety:** Move Z3 "Sort Errors" (bit-width/type
   mismatches) from runtime exceptions to compile-time errors using C++20
   template metaprogramming.
2. **Bit-growth arithmetic:** Automatically widen arithmetic result types to
   prevent silent overflow, making every truncation an explicit, reviewable
   decision.

## Scope

Z3 supports many SMT theories — unbounded integers, reals, arrays,
floating-point, uninterpreted functions — but Z3Wire intentionally covers only
**Booleans** and **fixed-width bit-vectors**.

The reason is in the name: hardware is built from *wires*. Every signal in a
digital circuit is either a single bit (Bool) or a bundle of bits with a known
width (bit-vector). Unbounded integers, reals, and other abstract mathematical
types do not correspond to physical hardware and are irrelevant to RTL modeling
and verification.

By targeting only the QF_BV logic (Quantifier-Free Bit-Vectors), Z3Wire can:
- Provide a tight, ergonomic API with no unused abstractions.
- Enforce hardware-meaningful constraints (fixed widths, explicit truncation).
- Keep the library small and auditable.

**Out of scope for this project:**
- Non-hardware SMT theories (unbounded integers, reals, arrays, floating-point,
  uninterpreted functions).
- Wrapping `z3::context` or `z3::solver`.

## Design Philosophy

- **Zero overhead:** Each wrapper stores only a `z3::expr`. No virtual
  functions, no extra data members.
- **Explicit over implicit:** No implicit conversions between signed/unsigned or
  different widths. Users must use the casting API to express intent.
- **Header-based template library.** The core types and operators are templates
  and must live in headers. Any non-template utilities should go in `.cc` files
  per Google C++ style guide conventions.

## Type System

The library centers around a zero-overhead wrapper class that holds a single
`z3::expr` by value, adding no runtime overhead.

| Type Alias    | Mapping               | Description                      |
| :------------ | :-------------------- | :------------------------------- |
| `z3w::Ubv<W>` | `BitVec<W, false>`   | Unsigned fixed-width bit-vector. |
| `z3w::Sbv<W>` | `BitVec<W, true>`    | Signed fixed-width bit-vector.   |
| `z3w::Bool`   | wrapper over `z3::expr` (Bool sort) | Symbolic boolean.  |

The core template is:

```cpp
template <size_t Width, bool IsSigned>
class BitVec {
    static_assert(Width > 0, "Bit-vector width must be at least 1.");
    z3::expr m_expr;
    // ...
};

template <size_t W> using Ubv = BitVec<W, false>;
template <size_t W> using Sbv = BitVec<W, true>;
```

Note: `BitVec<0, S>` is forbidden via `static_assert`. A zero-width bit-vector
has no meaning in hardware or SMT.

## Construction and Literals

### Compile-time range-checked literals

```cpp
template <uint64_t Value>
static BitVec<Width, IsSigned> Literal(z3::context& ctx);
```

Uses `static_assert` to verify the value fits in the specified width at compile
time.

```cpp
auto ok  = z3w::Ubv<8>::Literal<255>(ctx);  // Compiles
auto bad = z3w::Ubv<8>::Literal<256>(ctx);  // Compile error!
```

### Symbolic variables

```cpp
// Constructor taking a name creates a symbolic variable
z3w::Ubv<32> x(ctx, "x");
```

## Type Conversions

### Three-Tier Casting API

#### `z3w::cast<T>(val)` -- The Hardware Cast

Performs raw truncation, extension, or sign reinterpretation based on source and
target types. No safety checks. Use when you intentionally want hardware-style
overflow or wrap behavior.

Under the hood, uses `if constexpr` to select:
- Target width < source width: `z3::extract` (truncation).
- Target width > source width: `z3::zext` or `z3::sext` based on source
  signedness.
- Target width == source width: zero-overhead type reinterpretation (bitcast).

#### `z3w::safe_cast<T>(val)` -- The Compiler Guard

Only compiles if the cast is mathematically guaranteed to be lossless.

| Source      | Target      | Allowed?                                       |
| :---------- | :---------- | :--------------------------------------------- |
| `Ubv<W1>`   | `Ubv<W2>`   | Yes, if `W2 >= W1`.                            |
| `Sbv<W1>`   | `Sbv<W2>`   | Yes, if `W2 >= W1`.                            |
| `Ubv<W1>`   | `Sbv<W2>`   | Yes, if `W2 > W1` (needs 1 extra bit for sign).|
| `Sbv<W1>`   | `Ubv<W2>`   | **Always forbidden.** Negative values corrupt.  |
| Any         | Smaller     | **Always forbidden.** Truncation is not safe.   |

#### `z3w::checked_cast<T>(val)` -- The Verification Cast

Returns `std::pair<T, z3w::Bool>`. Performs the cast and also returns a symbolic
boolean formula representing whether mathematical data loss occurred. The user
can assert the boolean into the solver to verify safety.

```cpp
auto [result, overflowed] = z3w::checked_cast<z3w::Ubv<8>>(my_32bit_val);
solver.add(!overflowed.raw());  // Assert: this cast never loses data
```

### Bool / Ubv<1> Conversion

In Z3, `Bool` and a 1-bit bit-vector are distinct sorts. Hardware frequently
needs to convert between them (e.g., a condition flag in a register vs. a
logical condition). Z3Wire provides explicit conversion functions:

- **`z3w::to_bool(Ubv<1>)`** — converts a 1-bit vector to Bool (true if bit is
  1).
- **`z3w::to_ubv1(Bool)`** — converts a Bool to `Ubv<1>` (1 if true, 0 if
  false).

```cpp
z3w::Ubv<32> status(ctx, "status");
z3w::Bool ready = z3w::to_bool(z3w::extract<0, 0>(status));

z3w::Bool cond(ctx, "cond");
z3w::Ubv<1> flag = z3w::to_ubv1(cond);
```

## Bit Manipulation

### Bit Slicing (`extract`)

#### Fully static (compile-time bounds)

```cpp
template <size_t High, size_t Low, size_t InWidth, bool S>
auto extract(const BitVec<InWidth, S>& val);
// Returns z3w::Ubv<High - Low + 1>
```

Example: `auto opcode = z3w::extract<31, 24>(instruction);`

#### Symbolic offset + static width

```cpp
template <size_t TargetWidth, size_t InWidth, bool S, size_t IdxWidth>
z3w::Ubv<TargetWidth> extract(const BitVec<InWidth, S>& val,
                               const z3w::Ubv<IdxWidth>& start_idx);
```

Implemented via barrel-shifting: shift right by the symbolic offset, then
statically extract the bottom `TargetWidth` bits.

Example: `auto nibble = z3w::extract<4>(packet, symbolic_offset);`

### Concatenation (`concat`)

Glues bit-vectors together. The result width is `W1 + W2`, always returned as
`Ubv` (raw bits have no inherent signedness). Supports variadic arguments.

```cpp
z3w::Ubv<16> high(ctx, "high");
z3w::Ubv<16> low(ctx, "low");
auto full = z3w::concat(high, low);  // -> z3w::Ubv<32>
```

## Operations

### Arithmetic and Bitwise

To prevent silent overflows, Z3Wire adopts bit-growth semantics for arithmetic,
inspired by
[CIRCT HWArith](https://circt.llvm.org/docs/Dialects/HWArith/RationaleHWArith/).
All other operations enforce strict width and signedness matching.

- **Addition / Subtraction (`+`, `-`):** Result width is `max(W1, W2) + 1`.
  Operands are automatically sign-extended or zero-extended to match before the
  operation. (Bit growth.)
- **Bitwise logic (`&`, `|`, `^`):** Widths and signedness must match exactly.
  (Strict.)
- **Equality (`==`, `!=`):** Widths and signedness must match exactly. (Strict.)
- **Ordered comparison (`<`, `<=`, `>`, `>=`):** Widths and signedness must
  match exactly. Signedness-aware: dispatches to unsigned comparisons (`bvult`,
  `bvule`, `bvugt`, `bvuge`) for `Ubv` and signed comparisons (`bvslt`,
  `bvsle`, `bvsgt`, `bvsge`) for `Sbv`. Returns `z3w::Bool`. (Strict.)

Example:

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> b(ctx, "b");
auto sum = a + b;       // -> z3w::Ubv<9>
auto total = sum + a;   // -> z3w::Ubv<10>

// To model hardware overflow, explicitly truncate:
auto reg = z3w::cast<z3w::Ubv<8>>(total);
```

### Bool Operations

`z3w::Bool` supports standard logical operations:

- **Logical:** `&&`, `||`, `!`
- **Literals:** `z3w::Bool::True(ctx)`, `z3w::Bool::False(ctx)`

```cpp
z3w::Bool a(ctx, "a");
z3w::Bool b(ctx, "b");

z3w::Bool c = a && b;
z3w::Bool d = !a || b;
z3w::Bool t = z3w::Bool::True(ctx);
```

### Shifting

Z3Wire provides a three-tier shift API, mirroring the casting tiers.

#### `<<`, `>>` -- Hardware Shift

Raw hardware shift. Width stays constant, bits that shift out are silently lost.
Widths and signedness must match exactly (strict, like bitwise ops).

- **Left shift (`<<`):** Logical shift for both `Ubv` and `Sbv`.
- **Right shift (`>>`):** Logical shift (`lshr`) for `Ubv`, arithmetic shift
  (`ashr`) for `Sbv` (preserves the sign bit).

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> n(ctx, "n");
auto result = a << n;  // -> z3w::Ubv<8>, bits may be lost
```

#### `checked_shl`, `checked_shr` -- Checked Shift

Performs the shift and returns a symbolic Bool indicating whether any non-zero
bits were lost. Width stays constant.

```cpp
auto [shifted, lost] = z3w::checked_shl(a, n);
solver.add(!lost.raw());  // Assert: this shift never loses bits
```

#### `lossless_shl` -- Lossless Left Shift

Result type is wide enough to guarantee no bits are ever lost. Works with both
compile-time constant and symbolic shift amounts.

- **Constant shift `N`:** result width = `W + N`.
- **Symbolic shift `Ubv<K>`:** result width = `W + 2^K - 1` (assumes the
  maximum representable shift amount).

```cpp
z3w::Ubv<8> a(ctx, "a");

// Compile-time constant shift
auto r1 = z3w::lossless_shl<3>(a);    // -> z3w::Ubv<11>

// Symbolic shift
z3w::Ubv<3> n(ctx, "n");
auto r2 = z3w::lossless_shl(a, n);    // -> z3w::Ubv<15>
```

### Conditional Selection (`ite`)

Symbolic If-Then-Else. Works for any Z3Wire type (`Bool`, `Ubv<W>`, `Sbv<W>`).
Both branches must be the exact same type.

```cpp
template <typename T>
T ite(const z3w::Bool& cond, const T& true_val, const T& false_val);
```

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> b(ctx, "b");
z3w::Bool sel(ctx, "sel");

auto result = z3w::ite(sel, a, b);  // -> z3w::Ubv<8>
auto flag = z3w::ite(sel, z3w::Bool::True(ctx), z3w::Bool::False(ctx));
```

## Boundary Layer

- **`val.raw()`**: Returns the underlying `z3::expr` for interop with raw Z3
  APIs (e.g., passing to `z3::solver`).
- Users interact with `z3::context` and `z3::solver` directly; Z3Wire does not
  wrap these.

## MVP Scope

Supported in the initial version:
- Types: `Bool`, `Ubv<W>`, `Sbv<W>` (width > 0 enforced)
- Bool operations: `&&`, `||`, `!`, `True`/`False` literals
- Bool/Ubv<1> conversion: `to_bool`, `to_ubv1`
- Arithmetic: `+`, `-` (bit growth)
- Bitwise: `&`, `|`, `^`, `~` (strict width matching)
- Shifts: `<<`, `>>`, `checked_shl`, `checked_shr`, `lossless_shl`
- Equality: `==`, `!=`
- Ordered comparison: `<`, `<=`, `>`, `>=` (signedness-aware)
- Casting: `cast`, `safe_cast`, `checked_cast`
- Construction: `Literal<Value>`, symbolic variables
- Slicing: `extract` (static and symbolic-offset)
- Concatenation: `concat` (variadic)
- Conditional selection: `ite`

Deferred to post-MVP:
- Multiplication, division, modulo
