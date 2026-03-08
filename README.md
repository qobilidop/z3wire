# Z3Wire

[![CI](https://github.com/qobilidop/z3wire/actions/workflows/ci.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/ci.yml)

A type-safe C++20 template library for [Z3](https://github.com/Z3Prover/z3) that enforces compile-time bit-width consistency and bit-growth arithmetic for hardware modeling and formal verification.

## Features

- **Compile-time type safety** — bit-width and signedness mismatches become compiler errors, not runtime Z3 sort errors.
- **Bit-growth arithmetic** — `+` and `-` automatically widen the result to prevent silent overflow.
- **Three-tier casting** — `cast` (raw hardware), `safe_cast` (compile-time lossless), `checked_cast` (symbolic verification).
- **Three-tier shifting** — `<<`/`>>` (hardware), `checked_shl`/`checked_shr` (detect lost bits), `lossless_shl` (auto-widen result).
- **Zero overhead** — each wrapper holds only a `z3::expr`.

## Quick Example

```cpp
#include "z3wire.hpp"

z3::context ctx;
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> b(ctx, "b");

auto sum = a + b;  // z3w::Ubv<9>, no overflow possible

// Model hardware truncation explicitly
auto reg = z3w::cast<z3w::Ubv<8>>(sum);

// Verify the cast is safe
auto [result, overflowed] = z3w::checked_cast<z3w::Ubv<8>>(sum);
solver.add(!overflowed.raw());
```

## Building

Requires Bazel (via [Bazelisk](https://github.com/bazelbuild/bazelisk)):

```sh
bazel build //...
bazel test //...
```

## Design

See [docs/design.md](docs/design.md) for the full design document.

## License

See [LICENSE](LICENSE).
