# Roadmap

Future directions for Z3Wire.

## Operations

- **Multiply** (`*`) — Fundamental operation supported by all SMT bit-vector
    libraries. Design question: bit-growth semantics (result width = `W1 + W2`?)
    vs same-width with overflow detection.
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

## Release and distribution

- **First release (v0.1.0)** — Tag a stable version so users can depend on a
    snapshot rather than tracking `main`.
- **Publish to Bazel Central Registry** — Allow users to use
    `bazel_dep(name = "z3wire", version = "...")` without `git_override`.
- **vcpkg port** — Submit a port to the vcpkg registry so CMake users can depend
    on Z3Wire via `vcpkg.json`. Z3 already has a vcpkg port, so the dependency
    chain is straightforward.
- **Publish Weave to PyPI** — Package the Weave codegen tool for `pip install`.
    Separate from the C++ library since Weave is a development-time tool, not a
    runtime dependency.

## Weave

- **`Unpack()`** — Construct a symbolic struct from a flat bit-vector using
    `extract`.
- **Use `map` for repeated fields in proto** — Register arrays generate
    `repeated` proto fields today, which serialize every element including
    zeros. Switch to `map<uint32, RegType>` so only non-default entries are
    serialized. The C++ struct side stays as `std::array` (dense); the map is
    purely a serialization optimization.
- **Multi-file wire specs** — Support `include` directives so one wire spec can
    reference types from another. Requires dependency resolution in the
    resolver, cross-file `#include` generation, include search paths (`-I`
    flag), and a proper Bazel rule to handle transitive deps.

## Ergonomics

- **Solver interaction usage page** — Document where Z3Wire ends and raw Z3
    begins: `to_symbolic`, `to_concrete`, `.expr()`, and common solver patterns
    (assert, check, model extraction).

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

- **Clamp on truncation in `FromValue()`** — When `FromValue()` truncates,
    return the clamped value (min or max of the range) instead of the low-W-bit
    truncation. Clamping is more meaningful for a data holder type.

## Quality

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
