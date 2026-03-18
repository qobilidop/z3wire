# Roadmap

Future directions for Z3Wire, roughly in priority order.

## Recently completed

- **Concrete types (`UInt<W>`, `SInt<W>`)** — Fixed-width integer value holders
    that automatically promote in mixed expressions. See the
    [types guide](../usage/types.md#concrete-types).
- **Relaxed comparisons** — Comparison operators now allow different widths and
    signedness, automatically extending operands to a common type.
- **Unary negate** (`-x`) — Two's complement negation with bit-growth (width +
    1, always signed), consistent with binary subtraction.
- **Single-bit extraction** — `z3w::bit<N>(val)` shorthand for
    `z3w::extract<N, N>(val)`.
- **Combinational logic framing** — Documented the complete set of combinational
    logic primitives as the organizing principle for the API. See the
    [design overview](../design/overview.md#combinational-logic-primitives).
- **Weave codegen tool** — Generates C++ structs (symbolic + concrete) with full
    method implementations and protobuf messages from RDL bit field
    descriptions. See the [design doc](../design/weave.md).

## Next features

- **SymBool XOR** (`^`) — Dedicated XOR operator for `SymBool`. Currently
    expressible as `!=`, but a dedicated `^` would be more readable in
    logic-heavy code.
- **Reduction operators** (`reduce_and`, `reduce_or`, `reduce_xor`) — collapse
    all bits to a single `SymBool`. Low priority: `reduce_and` and `reduce_or`
    are expressible via comparisons (`x == ~0`, `x != 0`); `reduce_xor` (parity)
    is the only one that's hard to express today. Defer until a use case arises.
- **`SymUInt<1>` / `SymBool` friction** — Consider reducing friction between
    `SymUInt<1>` and `SymBool` (e.g., direct comparison), while respecting the
    "explicit over implicit" principle. Defer until real usage patterns emerge.

## Release and distribution

- **First release (v0.1.0)** — Tag a stable version so users can depend on a
    snapshot rather than tracking `main`.
- **Publish to Bazel Central Registry** — Allow users to use
    `bazel_dep(name = "z3wire", version = "...")` without `git_override`.

## Weave

- **`Unpack()`** — Construct a symbolic struct from a flat bit-vector using
    `extract`.

## Ergonomics

- **Reduce `.expr()` noise at solver boundaries** — Every `solver.add()` and
    `model.eval()` call requires `.expr()` to unwrap Z3Wire types. A thin
    `z3w::Solver` wrapper (or free-function helpers like `z3w::eval()`) could
    accept Z3Wire types directly, delegating to `z3::solver` under the hood.
- **Shorter literal construction** — `SymUInt<2>::Literal<0>(ctx)` is verbose
    for a common operation. Consider a free-function shorthand (e.g.,
    `z3w::lit<SymUInt<2>, 0>(ctx)`) or dedicated helpers.
- **Scoped solver checks** — The push/add/check/pop pattern is repeated in every
    verification. A helper like `z3w::check(solver, constraint)` could
    encapsulate this. Trades explicitness for brevity — needs careful design.
- **Symbolic struct construction helpers** — Creating structs of symbolic types
    requires threading `z3::context&` through every field. Investigate
    lightweight helpers (e.g., a named-field factory) once real usage patterns
    emerge.

## Quality

- **Improve test coverage** — The [test coverage](test-coverage.md) tracks which
    template instantiations are tested. Key gaps: signed (`SymSInt`/`SInt`)
    operations are under-tested across the board, W=1 and W=64 boundary cases
    are sparse, and mixed concrete+symbolic bitwise only tests AND.
