# Z3Wire

[![Bazel](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml)
[![CMake](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml)
[![Checks](https://github.com/qobilidop/z3wire/actions/workflows/checks.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/checks.yml)
[![codecov](https://codecov.io/gh/qobilidop/z3wire/graph/badge.svg)](https://codecov.io/gh/qobilidop/z3wire)
[![Docs](https://github.com/qobilidop/z3wire/actions/workflows/docs.yml/badge.svg)](https://qobilidop.github.io/z3wire/)

<!-- docs-start -->

Type-safe Z3 bit-vectors for hardware verification. C++20 and above.

## Why Z3Wire?

Using Z3 bit-vectors directly for hardware verification is error-prone:

- **Width mismatches are silent.** Adding a 32-bit vector to an 8-bit vector
  compiles fine but crashes at runtime with `Z3_SORT_ERROR`.
- **Signedness is unchecked.** Comparing bit-vectors requires choosing the right
  function (`z3::ult` vs `z3::slt`), but nothing prevents calling the wrong one.
- **Overflow requires vigilance.** Arithmetic silently wraps by default. Z3
  provides overflow predicates (`bvadd_no_overflow`, etc.), but they are opt-in
  and easy to forget — a missed check means a proof may pass because the formula
  lost information, not because the design is correct.

Z3Wire solves these by bringing hardware semantics into the type system:

- **Compile-time type safety** — width and signedness mismatches become compile-time
  errors, not runtime surprises.
- **Bit-growth arithmetic** — results widen automatically, making every
  truncation an explicit, reviewable decision.

## What's in the name?

The name reflects the scope: hardware is built from *wires*. Every signal in a
digital circuit is either a single bit or a bundle of bits with a known width.
Z3Wire wraps Z3 with type-safe Booleans and fixed-width bit-vectors, covering
the complete set of combinational logic primitives that operate on these wires.

## Features

- **Three-tier casting** — `cast` (raw hardware), `safe_cast` (compile-time
  lossless), `checked_cast` (symbolic verification).
- **Three-tier shifting** — `<<`/`>>` (hardware), `checked_shl`/`checked_shr`
  (detect lost bits), `lossless_shl` (auto-widen result).
- **Zero overhead** — each wrapper holds only a `z3::expr`.

## Quick example

```cpp
#include <z3++.h>
#include "z3wire/bitvec.h"

z3::context ctx;
z3::solver solver(ctx);

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

Z3Wire supports both **Bazel** and **CMake** build systems.

### Devcontainer (recommended)

The easiest way to build and test is with the included devcontainer, which has
all dependencies pre-installed:

```sh
./dev.sh bazel build //...
./dev.sh bazel test //...
./dev.sh bazel run //examples:safe_adder
```

`dev.sh` builds the devcontainer image on first use and runs commands inside it
with a persistent Bazel cache.

### Bazel

```sh
bazel build //...
bazel test //...
```

The first build takes several minutes (compiling Z3 from source). Subsequent
builds use the cached result and complete in seconds.

### CMake

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

Requires clang and Z3 to be installed on your system (e.g., `apt install clang libz3-dev`).

## Documentation

Visit [qobilidop.github.io/z3wire](https://qobilidop.github.io/z3wire/) for the
full documentation, including getting started guide, user guide, and examples.

## License

See [LICENSE](https://github.com/qobilidop/z3wire/blob/main/LICENSE).

<!-- docs-end -->
