# Roadmap

Future directions for Z3Wire, roughly in priority order.

## Recently completed

- **Concrete types (`UInt<W>`, `SInt<W>`)** — Fixed-width integer types that
  mirror the symbolic API and automatically promote in mixed expressions.
  See the [concrete types guide](../user/concrete-types.md).
- **Relaxed comparisons** — Comparison operators now allow different widths
  and signedness, automatically extending operands to a common type.

## Next features

- **Complete combinational logic primitives** — Z3Wire aims to cover the
  complete set of combinational logic operations. Two gaps remain:
  - *Unary negate* (`-x`) — two's complement negation.
  - *Reduction operators* (`reduce_and`, `reduce_or`, `reduce_xor`) — collapse
    all bits to a single `Bool`.
- **Document the combinational logic framing** — Update design docs and README
  to make explicit that Z3Wire's scope is the complete set of combinational
  logic primitives.

## Release and distribution

- **First release (v0.1.0)** — Tag a stable version so users can depend on a
  snapshot rather than tracking `main`.
- **Publish to Bazel Central Registry** — Allow users to use
  `bazel_dep(name = "z3wire", version = "...")` without `git_override`.

## Quality

- **Improve test coverage** — The [coverage checklist](coverage.md) tracks
  which template instantiations are tested. Key gaps: signed (`Sbv`/`SInt`)
  operations are under-tested across the board, W=1 and W=64 boundary cases
  are sparse, and mixed concrete+symbolic bitwise only tests AND.
