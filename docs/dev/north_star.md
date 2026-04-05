# North Star

This document defines what Z3Wire is, what it isn't, and what "done" looks like.

## What Z3Wire is

A type-safe C++20 template library wrapping Z3's Bool and bit-vector types for
hardware verification.

Named for the wires in digital circuits: single bits and fixed-width bundles,
now type-safe.

## Scope

Z3Wire covers **Booleans** and **fixed-width bit-vectors** only (the QF_BV
logic). Every signal in a digital circuit is either a single bit or a bundle of
bits with a known width - these are the only types Z3Wire needs.

### Non-goals

- Non-hardware SMT theories (unbounded integers, reals, arrays, floating-point,
    uninterpreted functions).
- Wrapping `z3::context` or `z3::solver` - Z3Wire is a type layer, not a solver
    framework.
- Sequential semantics (clocks, state, memory) - Z3Wire is combinational only.

## Design principles

- **Zero overhead:** Each wrapper stores only a `z3::expr`. No virtual
    functions, no extra data members.
- **Explicit over implicit:** No implicit conversions between signed/unsigned or
    different widths. Users must use the casting API to express intent.
- **Bit-growth arithmetic:** Results widen automatically, making every
    truncation an explicit, reviewable decision.
- **Combinational only:** Standard combinational logic primitives (bitwise,
    arithmetic, shifting, mux, extract, concat) belong in Z3Wire. Sequential
    semantics do not.
- **Header-based template library:** Core types and operators are templates and
    live in headers. Non-template utilities go in `.cc` files.

## Definition of "done"

Z3Wire is complete when it provides type-safe wrappers for all standard
combinational operations on Booleans and fixed-width bit-vectors, with both
symbolic (Z3-backed) and concrete (native value) variants.

### Operations

- Logical: `!`, `&&`, `||`, `^` (on SymBool/Bool)
- Bitwise: `~`, `&`, `|`, `^`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Arithmetic: `+`, `-`, unary `-`, `*`, `/`, `%`
- Shifting: `shl`, `shr` (static and dynamic)
- Rotation: `rotl`, `rotr` (static and dynamic)
- Bit manipulation: `extract`, `replace`, `concat`, `repeat`
- Reduction: `reduce_and`, `reduce_or`, `reduce_xor`
- Conditional: `ite`

### Type system

- Symbolic types: `SymBool`, `SymUInt<W>`, `SymSInt<W>`
- Concrete types: `Bool`, `UInt<W>`, `SInt<W>`
- Wide concrete types (W > 64) using byte arrays
- Three-tier casting: `safe_cast`, `checked_cast`, `unsafe_cast`
- Signedness reinterpretation: `as_unsigned()`, `as_signed()`
- Sort conversion: `as_bool()`, `as_uint1()`
- Domain conversion: `to_symbolic()`, `to_concrete()`

### Quality

- Compile-fail tests for every `static_assert` guard
- Unit tests for every operation
- Fuzz tests for key properties

### Distribution

- Tagged release (v0.1.0+)
- Published to Bazel Central Registry
- vcpkg port

### Documentation

- Usage docs for all operations and type conversions
- Examples demonstrating real verification scenarios
- Design docs explaining key decisions
- Development docs for contributors
