# Design Principles

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

**Out of scope:**

- Non-hardware SMT theories (unbounded integers, reals, arrays, floating-point,
    uninterpreted functions).
- Wrapping `z3::context` or `z3::solver`.

## Principles

- **Zero overhead:** Each wrapper stores only a `z3::expr`. No virtual
    functions, no extra data members.
- **Explicit over implicit:** No implicit conversions between signed/unsigned or
    different widths. Users must use the casting API to express intent.
- **Combinational only:** If an operation is a standard combinational logic
    primitive (bitwise, arithmetic, shifting, mux, extract, concat), it belongs
    in Z3Wire. If it requires sequential semantics (clocks, state, memory), it
    is out of scope.
- **Header-based template library.** The core types and operators are templates
    and must live in headers. Any non-template utilities should go in `.cc`
    files per Google C++ style guide conventions.
