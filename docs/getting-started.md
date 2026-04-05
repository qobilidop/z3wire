# Getting Started

A reading guide for Z3Wire's documentation.

## What Z3Wire is

Z3Wire provides type-safe Booleans and fixed-width bit-vectors for combinational
logic verification with Z3. Width, signedness, and overflow safety are enforced
at compile time. See the [home page](index.md) for a comparison with raw Z3.

## Recommended reading order

### 1. Learn the types

Start with [Types](usage/types.md) to see the symbolic and concrete types Z3Wire
provides. Then read [Type Conversions](usage/type-conversions.md) to understand
how values move between types — casting, promotion, and the symbolic/concrete
boundary.

### 2. See the operations

[Operations](usage/operations.md) covers everything you can do with Z3Wire
types: arithmetic, bitwise, comparison, shifting, and bit manipulation. Each
operation includes typing rules and examples.

### 3. Explore the examples

The [examples](https://github.com/qobilidop/z3wire/tree/main/examples) are
self-contained programs you can run. Start with the adder — it builds a
gate-level ripple-carry adder and proves it matches Z3Wire's bit-growth
arithmetic, showing the core verification pattern.

### 4. Understand the design (optional)

The [Design](design/index.md) section explains the reasoning behind Z3Wire's
type system decisions. Start here if you're wondering *why* things work the way
they do, or if you're evaluating Z3Wire for a project.

### 5. Contribute (optional)

The [Development](dev/index.md) section covers setup, build commands, style
conventions, and workflow. Start with [Setup](dev/setup.md) to get a dev
environment running, then read the [North Star](dev/north_star.md) and
[Roadmap](dev/roadmap.md) to understand the project's direction.
