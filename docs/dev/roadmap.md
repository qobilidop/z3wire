# Roadmap

Future directions for Z3Wire, roughly in priority order.

## Recently completed

- **Concrete types (`UInt<W>`, `SInt<W>`)** — Fixed-width integer types that
  mirror the symbolic API and automatically promote in mixed expressions.
  See the [concrete types guide](../user/concrete-types.md).
- **Relaxed comparisons** — Comparison operators now allow different widths
  and signedness, automatically extending operands to a common type.

## Next features

- **Multiplication, division, modulo** — Deferred from MVP. Need to decide on
  bit-growth semantics (e.g., multiplication result width = W1 + W2).

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
