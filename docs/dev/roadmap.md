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

- **Compare symbolic bit-vectors with native integers** — Allow
    `ethertype == 0x0800` instead of `ethertype == UInt<16>::Literal<0x0800>()`.
    The native integer converts losslessly to a `BitVec` at its native width,
    then the existing auto-widening comparison handles the rest. No lossy
    conversion occurs at any point. Scoped to comparisons only (`==`, `!=`, `<`,
    `<=`, `>`, `>=`) since arithmetic and bitwise with raw integers would
    produce surprising result types. Blocked on a broader review of the
    auto-promotion strategy.

- **Symbolic struct construction helpers** — Creating structs of symbolic types
    requires threading `z3::context&` through every field. Investigate
    lightweight helpers (e.g., a named-field factory) once real usage patterns
    emerge.

- **Saturating cast operation** — If callers ever need saturating-narrow
    semantics (clamp value to the representable range instead of
    bit-truncation), expose it as an explicit `saturating_cast<T>(v)` operation
    rather than as the silent default of `From*`/`TryFrom*`. Bit-truncation is
    the right default for a hardware verification library; saturation is a
    different operation that should be explicitly requested. Defer until a real
    use case appears.

### Quality

- **Improve test coverage** — Key gaps: signed (`SymSInt`/`SInt`) operations are
    under-tested across the board, W=1 and W=64 boundary cases are sparse, and
    mixed concrete+symbolic bitwise only tests AND.

- **Fuzz tests** — Property-based tests using Google FuzzTest. Roundtrip
    (`concrete -> symbolic -> concrete`) is done. Ideas for more:

    - **Checked construction idempotency** — `TryFrom(v.value())` on a valid
        `BitVec` should return a `TryFromResult` with `value == v` and
        `truncated == false`. No Z3 needed.
    - **Cast roundtrip** — Same-width `unsafe_cast<UInt>(unsafe_cast<SInt>(v))`
        preserves bits.
    - **Arithmetic properties** — Commutativity (`a + b == b + a`), negation
        (`a + (-a) == 0`), bitwise identity (`a & a == a`). Verified via solver.
    - **Shift properties** — `shl<N>(shr<N>(val))` masks lower bits for unsigned.
        Verified via solver.
    - **Continuous fuzzing CI** — Scheduled GitHub Actions workflow running
        `--config=fuzztest` with longer timeouts for deeper coverage.
