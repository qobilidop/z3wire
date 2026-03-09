# Getting started

## Prerequisites

Z3Wire requires:

- A C++20-compatible compiler (clang is the only supported compiler; GCC may work but is untested)
- [Z3](https://github.com/Z3Prover/z3)
- Either [Bazel](https://bazel.build/) (via
  [Bazelisk](https://github.com/bazelbuild/bazelisk)) or
  [CMake](https://cmake.org/) 3.16+

## Using the devcontainer

The easiest way to get started is with the included devcontainer, which has all
dependencies pre-installed:

```sh
git clone https://github.com/qobilidop/z3wire.git
cd z3wire
./dev.sh bazel build //...
```

`dev.sh` builds the devcontainer image on first use and runs commands inside it
with a persistent Bazel cache.

!!! note
    The first build takes several minutes (compiling Z3 from source). Subsequent
    builds use the cached result and complete in seconds.

## Adding Z3Wire to your project

### Bazel

Add Z3Wire as a dependency in your `MODULE.bazel`:

```python
bazel_dep(name = "z3wire")

# Use a git override to pin to a specific commit:
git_override(
    module_name = "z3wire",
    remote = "https://github.com/qobilidop/z3wire.git",
    commit = "<commit-hash>",
)
```

Then depend on it in your `BUILD.bazel`:

```python
cc_binary(
    name = "my_verifier",
    srcs = ["my_verifier.cc"],
    deps = ["@z3wire//z3wire"],
)
```

### CMake

Use `FetchContent` to add Z3Wire to your project:

```cmake
include(FetchContent)
FetchContent_Declare(
    z3wire
    GIT_REPOSITORY https://github.com/qobilidop/z3wire.git
    GIT_TAG main
)
FetchContent_MakeAvailable(z3wire)

target_link_libraries(my_verifier PRIVATE z3wire::z3wire)
```

Z3 must be installed on your system (e.g., `apt install libz3-dev`).
Alternatively, if Z3 is installed in a custom location:

```sh
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/z3
```

## Your first verification

Here's a minimal example that verifies a property of 8-bit addition:

```cpp
#include <z3++.h>
#include <iostream>
#include "z3wire/bitvec.h"

int main() {
    z3::context ctx;
    z3::solver solver(ctx);

    // Create two symbolic 8-bit unsigned values.
    z3w::Ubv<8> a(ctx, "a");
    z3w::Ubv<8> b(ctx, "b");

    // Bit-growth addition: result is 9 bits, no overflow possible.
    auto sum = a + b;  // z3w::Ubv<9>

    // Verify: the 9-bit sum is always >= both operands.
    // (This is trivially true because no overflow can occur.)
    auto a_wide = z3w::safe_cast<z3w::Ubv<9>>(a);
    solver.add((sum < a_wide).raw());

    if (solver.check() == z3::unsat) {
        std::cout << "Verified: 9-bit sum is always >= both inputs.\n";
    } else {
        std::cout << "Bug found!\n";
    }
}
```

## Next steps

- [Type System](user/types.md) — learn about `Bool`, `Ubv<W>`, and `Sbv<W>`
- [Operations](user/operations.md) — arithmetic, bitwise, and comparison
- [Examples](examples/safe-adder.md) — runnable verification examples
