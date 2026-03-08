# Roadmap

Future directions for Z3Wire, roughly in priority order.

## Next features

- **Multiplication, division, modulo** — Deferred from MVP. Need to decide on
  bit-growth semantics (e.g., multiplication result width = W1 + W2).

## Release and distribution

- **First release (v0.1.0)** — Tag a stable version so users can depend on a
  snapshot rather than tracking `main`.
- **Publish to Bazel Central Registry** — Allow users to use
  `bazel_dep(name = "z3wire", version = "...")` without `git_override`.

## Quality

- **Improve test coverage** — Review Codecov reports and fill gaps.
