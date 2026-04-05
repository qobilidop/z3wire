# Compile-Fail Tests

## Purpose

Verify that compile-time guards (`static_assert`, SFINAE) correctly reject
invalid code. Each test contains a single snippet that should fail to compile,
and the test asserts that compilation fails with the expected error message.

## Directory structure

```
tests/compile_fail/
  defs.bzl          # cc_compile_fail_test Starlark rule
  BUILD.bazel       # test targets
  *_test.cc         # one file per guard
```

## Rule

`cc_compile_fail_test` is a custom Starlark rule defined in `defs.bzl` that:

1. Gets the C++ toolchain and include paths from `deps` via `CcInfo`.
1. Runs the compiler on the source file as a build action.
1. Inverts the exit code - compilation failure = test passes.
1. Matches `expected_message` in compiler stderr to prevent false positives.

## Usage

```python
load(":defs.bzl", "cc_compile_fail_test")

cc_compile_fail_test(
    name = "sym_bit_vec_zero_width_test",
    src = "sym_bit_vec_zero_width_test.cc",
    deps = ["//z3wire:sym_bit_vec"],
    expected_message = "Bit-vector width must be at least 1",
)
```

## Test file example

```cpp
// sym_bit_vec_zero_width_test.cc
#include "z3wire/sym_bit_vec.h"

// SymBitVec<0, false> should fail: "Bit-vector width must be at least 1."
template class z3w::SymBitVec<0, false>;
```

## Coverage

One test for each of the ~24 `static_assert` guards plus the SFINAE constraint
on `Int::FromValue(int64_t)`.
