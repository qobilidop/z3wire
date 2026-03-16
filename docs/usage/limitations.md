# Limitations

Known limitations of the current implementation.

## Scope

- **Booleans and fixed-width bit-vectors only.** Z3Wire wraps the QF_BV
    (quantifier-free bit-vector) fragment of Z3. Unbounded integers, reals,
    arrays, floating-point, and uninterpreted functions are not supported.
- **No multiplication, division, or modulo.** These arithmetic operations are
    planned but not yet implemented. See the [roadmap](../dev/roadmap.md).
- **No context or solver wrappers.** Users interact with `z3::context` and
    `z3::solver` directly. Z3Wire wraps only expressions.

## Concrete types

- **Bit-width limited to 1--64.** Concrete types (`UInt<W>`, `SInt<W>`)
    require `W <= 64` because they are backed by native C++ integers. Symbolic
    types (`SymUInt<W>`, `SymSInt<W>`) have no such restriction.
- **Value holders only.** Concrete types support construction, `.value()`,
    and equality (`==`, `!=`). For arithmetic, bitwise, shifts, and other
    operations, use symbolic types or native C++ integers via `.value()`.

## Compiler support

- **Clang only.** GCC is untested and not supported.
