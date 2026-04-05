# Roadmap

## Operations

- **Division and remainder** (`/`, `%`) — `udiv`, `sdiv`, `urem`, `srem`.
    Signedness-aware type system makes the API design interesting: signed vs
    unsigned division can be selected by operand types rather than separate
    function names.
- **Repeat** — Replicate a bit pattern N times. Useful for mask generation.
- **Reduction operators** (`reduce_and`, `reduce_or`, `reduce_xor`) — Collapse
    all bits to a single `SymBool`. Low priority: `reduce_and` and `reduce_or`
    are expressible via comparisons (`x == ~0`, `x != 0`); `reduce_xor` (parity)
    is the only one that's hard to express today. Defer until a use case arises.
- **`SymUInt<1>` / `SymBool` friction** — Consider reducing friction between
    `SymUInt<1>` and `SymBool` (e.g., direct comparison), while respecting the
    "explicit over implicit" principle. Defer until real usage patterns emerge.

### Release and distribution

- **First release (v0.1.0)** — Tag a stable version so users can depend on a
    snapshot rather than tracking `main`.
- **Publish to Bazel Central Registry** — Allow users to use
    `bazel_dep(name = "z3wire", version = "...")` without `git_override`.
- **vcpkg port** — Submit a port to the vcpkg registry so CMake users can depend
    on Z3Wire via `vcpkg.json`. Z3 already has a vcpkg port, so the dependency
    chain is straightforward.

### Ergonomics

- **Solver interaction usage page** — Document where Z3Wire ends and raw Z3
    begins: `to_symbolic`, `to_concrete`, `.expr()`, and common solver patterns
    (assert, check, model extraction).

- **Shorter literal construction** — `SymUInt<2>::Literal<0>(ctx)` is verbose
    for a common operation. Consider a free-function shorthand (e.g.,
    `z3w::lit<SymUInt<2>, 0>(ctx)`) or dedicated helpers.

- **Symbolic struct construction helpers** — Creating structs of symbolic types
    requires threading `z3::context&` through every field. Investigate
    lightweight helpers (e.g., a named-field factory) once real usage patterns
    emerge.

- **Clamp on truncation in `FromValue()`** — When `FromValue()` truncates,
    return the clamped value (min or max of the range) instead of the low-W-bit
    truncation. Clamping is more meaningful for a data holder type.

### Quality

- **Improve test coverage** — Key gaps: signed (`SymSInt`/`SInt`) operations are
    under-tested across the board, W=1 and W=64 boundary cases are sparse, and
    mixed concrete+symbolic bitwise only tests AND.

- **Fuzz tests** — Property-based tests using Google FuzzTest. Roundtrip
    (`concrete -> symbolic -> concrete`) is done. Ideas for more:

    - **Checked construction idempotency** — `FromValue(v.value())` on a valid
        `BitVec` should return `{v, false}`. No Z3 needed.
    - **Cast roundtrip** — Same-width `unsafe_cast<UInt>(unsafe_cast<SInt>(v))`
        preserves bits.
    - **Arithmetic properties** — Commutativity (`a + b == b + a`), negation
        (`a + (-a) == 0`), bitwise identity (`a & a == a`). Verified via solver.
    - **Shift properties** — `shl<N>(shr<N>(val))` masks lower bits for unsigned.
        Verified via solver.
    - **Continuous fuzzing CI** — Scheduled GitHub Actions workflow running
        `--config=fuzztest` with longer timeouts for deeper coverage.
