# Z3Wire

[![Bazel](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/bazel.yml)
[![CMake](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/cmake.yml)
[![Checks](https://github.com/qobilidop/z3wire/actions/workflows/checks.yml/badge.svg)](https://github.com/qobilidop/z3wire/actions/workflows/checks.yml)
[![codecov](https://codecov.io/gh/qobilidop/z3wire/graph/badge.svg)](https://codecov.io/gh/qobilidop/z3wire)
[![Docs](https://github.com/qobilidop/z3wire/actions/workflows/docs.yml/badge.svg)](https://qobilidop.github.io/z3wire/)

<!-- docs-start -->
Z3 bit-vectors with compile-time width safety and overflow-proof arithmetic. A C++20 template library.

## Why Z3Wire?

Z3's C++ API represents all expressions as `z3::expr`, with no compile-time
distinction between Booleans and bit-vectors, no width tracking, and no
signedness information. This means:

- Adding a 32-bit vector to an 8-bit vector **compiles silently** but crashes at
  runtime with `Z3_SORT_ERROR`.
- Comparing bit-vectors requires choosing the right function (`z3::ult` vs
  `z3::slt`), but nothing prevents calling the wrong one.
- Arithmetic silently discards overflow — adding two 8-bit values gives an 8-bit
  result, losing the carry bit.

Z3Wire fixes these problems:

- **Compile-time type safety** catches width and signedness mismatches as
  compiler errors.
- **Bit-growth arithmetic** widens results automatically, making every
  truncation an explicit, reviewable decision.

## Who is this for?

Hardware designers, verification engineers, and security researchers who use Z3
for formal verification of digital circuits and need type safety guarantees that
raw Z3 does not provide.

## Features

- **Compile-time type safety** — bit-width and signedness mismatches become
  compiler errors, not runtime Z3 sort errors.
- **Bit-growth arithmetic** — `+` and `-` automatically widen the result to
  prevent silent overflow.
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
