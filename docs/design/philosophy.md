# Design Philosophy

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

Furthermore, Z3's arithmetic operates at fixed widths - adding two 8-bit vectors
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

For project scope, design principles, and non-goals, see the
[North Star](../dev/north_star.md).
