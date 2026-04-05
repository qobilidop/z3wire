# North Star

## What Z3Wire is

Z3Wire provides type-safe Booleans and fixed-width bit-vectors for combinational
logic verification with Z3. Width, signedness, and overflow safety are enforced
at compile time.

## What Z3Wire is not

- A solver framework - does not wrap `z3::context` or `z3::solver`.
- A general-purpose SMT library - only Booleans and fixed-width bit-vectors.
- A sequential logic library - no clocks, state, or memory.

## Design principles

- **Minimal overhead:** Symbolic types carry only a `z3::expr` at runtime.
- **No lossy implicit conversions:** Lossy conversions require explicit casts.
    Lossless conversions happen automatically.
