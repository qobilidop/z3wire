# Z3Wire Design Document

## Project Identity

- **Name:** Z3Wire
- **Namespace:** `z3w::`
- **One-liner:** A type-safe, header-only C++20 abstraction layer for Z3 that
  enforces compile-time bit-width consistency and precision-preserving "natural
  growth" arithmetic for hardware modeling and formal verification.
- **Target audience:** Hardware designers, verification engineers, and security
  researchers.
- **Theory focus:** Strictly limited to Bit-Vectors (BV) and Booleans.

## Core Goal

Move Z3 "Sort Errors" (bit-width/type mismatches) from runtime exceptions to
compile-time errors using C++20 template metaprogramming.

## Type System

The library centers around a zero-overhead wrapper class that holds a single
`z3::expr` by value, adding no runtime overhead.

| Type Alias    | Mapping               | Description                      |
| :------------ | :-------------------- | :------------------------------- |
| `z3w::ubv<W>` | `BitVec<W, false>`   | Unsigned fixed-width bit-vector. |
| `z3w::sbv<W>` | `BitVec<W, true>`    | Signed fixed-width bit-vector.   |
| `z3w::Bool`   | wrapper over `z3::expr` (Bool sort) | Symbolic boolean.  |

The core template is:

```cpp
template <size_t Width, bool IsSigned>
class BitVec {
    z3::expr m_expr;
    // ...
};

template <size_t W> using ubv = BitVec<W, false>;
template <size_t W> using sbv = BitVec<W, true>;
```

## "Natural Growth" Arithmetic

To prevent silent overflows, Z3Wire adopts precision-preserving semantics
inspired by [CIRCT HWArith](https://circt.llvm.org/docs/Dialects/HWArith/RationaleHWArith/).

- **Addition / Subtraction (`+`, `-`):** Result width is `max(W1, W2) + 1`.
  Operands are automatically sign-extended or zero-extended to match before the
  operation.
- **Bitwise logic (`&`, `|`, `^`):** Strictly enforced. Widths and signedness
  must match exactly (`static_assert`).
- **Relational (`==`, `!=`):** Strictly enforced matching widths and signedness.

Example:

```cpp
z3w::ubv<8> a(ctx, "a");
z3w::ubv<8> b(ctx, "b");
auto sum = a + b;       // -> z3w::ubv<9>
auto total = sum + a;   // -> z3w::ubv<10>

// To model hardware overflow, explicitly truncate:
auto reg = z3w::cast<z3w::ubv<8>>(total);
```

## Shifting

- **Left shift (`<<`):** Logical shift for both `ubv` and `sbv`. Width remains
  constant.
- **Right shift (`>>`):** Logical shift (`lshr`) for `ubv`, arithmetic shift
  (`ashr`) for `sbv` (preserves the sign bit).
- The shift amount is automatically cast to match the LHS width, since Z3
  requires matching widths for shift operations.

## Three-Tier Casting API

### `z3w::cast<T>(val)` -- The Hardware Cast

Performs raw truncation, extension, or sign reinterpretation based on source and
target types. No safety checks. Use when you intentionally want hardware-style
overflow or wrap behavior.

Under the hood, uses `if constexpr` to select:
- Target width < source width: `z3::extract` (truncation).
- Target width > source width: `z3::zext` or `z3::sext` based on source
  signedness.
- Target width == source width: zero-overhead type reinterpretation (bitcast).

### `z3w::safe_cast<T>(val)` -- The Compiler Guard

Only compiles if the cast is mathematically guaranteed to be lossless.

| Source      | Target      | Allowed?                                       |
| :---------- | :---------- | :--------------------------------------------- |
| `ubv<W1>`   | `ubv<W2>`   | Yes, if `W2 >= W1`.                            |
| `sbv<W1>`   | `sbv<W2>`   | Yes, if `W2 >= W1`.                            |
| `ubv<W1>`   | `sbv<W2>`   | Yes, if `W2 > W1` (needs 1 extra bit for sign).|
| `sbv<W1>`   | `ubv<W2>`   | **Always forbidden.** Negative values corrupt.  |
| Any         | Smaller     | **Always forbidden.** Truncation is not safe.   |

### `z3w::checked_cast<T>(val)` -- The Verification Cast

Returns `std::pair<T, z3w::Bool>`. Performs the cast and also returns a symbolic
boolean formula representing whether mathematical data loss occurred. The user
can assert the boolean into the solver to verify safety.

```cpp
auto [result, overflowed] = z3w::checked_cast<z3w::ubv<8>>(my_32bit_val);
solver.add(!overflowed.raw());  // Assert: this cast never loses data
```

## Construction and Literals

### Compile-time range-checked literals

```cpp
template <uint64_t Value>
static BitVec<Width, IsSigned> Literal(z3::context& ctx);
```

Uses `static_assert` to verify the value fits in the specified width at compile
time.

```cpp
auto ok  = z3w::ubv<8>::Literal<255>(ctx);  // Compiles
auto bad = z3w::ubv<8>::Literal<256>(ctx);  // Compile error!
```

### Symbolic variables

```cpp
// Constructor taking a name creates a symbolic variable
z3w::ubv<32> x(ctx, "x");
```

## Bit Slicing (`extract`)

### Fully static (compile-time bounds)

```cpp
template <size_t High, size_t Low, size_t InWidth, bool S>
auto extract(const BitVec<InWidth, S>& val);
// Returns z3w::ubv<High - Low + 1>
```

Example: `auto opcode = z3w::extract<31, 24>(instruction);`

### Symbolic offset + static width

```cpp
template <size_t TargetWidth, size_t InWidth, bool S, size_t IdxWidth>
z3w::ubv<TargetWidth> extract(const BitVec<InWidth, S>& val,
                               const z3w::ubv<IdxWidth>& start_idx);
```

Implemented via barrel-shifting: shift right by the symbolic offset, then
statically extract the bottom `TargetWidth` bits.

Example: `auto nibble = z3w::extract<4>(packet, symbolic_offset);`

## Concatenation (`concat`)

Glues bit-vectors together. The result width is `W1 + W2`, always returned as
`ubv` (raw bits have no inherent signedness). Supports variadic arguments.

```cpp
z3w::ubv<16> high(ctx, "high");
z3w::ubv<16> low(ctx, "low");
auto full = z3w::concat(high, low);  // -> z3w::ubv<32>
```

## Multiplexing (`ite`)

Symbolic If-Then-Else for building hardware branch logic.

```cpp
template <size_t W, bool S>
auto ite(const z3w::Bool& cond,
         const BitVec<W, S>& true_val,
         const BitVec<W, S>& false_val);
// Returns BitVec<W, S>
```

Both branches must be the exact same type.

## Boundary Layer

- **`val.raw()`**: Returns the underlying `z3::expr` for interop with raw Z3
  APIs (e.g., passing to `z3::solver`).
- Users interact with `z3::context` and `z3::solver` directly; Z3Wire does not
  wrap these.

## Design Philosophy

- **Zero overhead:** Each wrapper stores only a `z3::expr`. No virtual
  functions, no extra data members.
- **Minimalism:** MVP excludes multiplication and division. These can be added
  later (multiplication with natural growth: result width = `W1 + W2`; division
  is rare in hardware and hard for SMT solvers).
- **Explicit over implicit:** No implicit conversions between signed/unsigned or
  different widths. Users must use the casting API to express intent.
- **Header-only C++20 library.**

## MVP Scope

Supported in the initial version:
- Types: `Bool`, `ubv<W>`, `sbv<W>`
- Arithmetic: `+`, `-` (natural growth)
- Bitwise: `&`, `|`, `^`, `~` (strict width matching)
- Shifts: `<<`, `>>` (signedness-aware)
- Relational: `==`, `!=`
- Casting: `cast`, `safe_cast`, `checked_cast`
- Construction: `Literal<Value>`, symbolic variables
- Slicing: `extract` (static and symbolic-offset)
- Concatenation: `concat` (variadic)
- Control flow: `ite`

Explicitly excluded from MVP:
- Multiplication, division, modulo
- Z3 unbounded integers, arrays, floating-point, uninterpreted functions
- Wrapping `z3::context` or `z3::solver`
