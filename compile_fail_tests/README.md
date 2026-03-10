# Compile-Fail Tests

## Purpose

Verify that compile-time guards (`static_assert`, SFINAE) correctly reject
invalid code. Each test contains a single snippet that should fail to compile,
and the test asserts that compilation fails with the expected error message.

## Directory structure

```
compile_fail_tests/
  defs.bzl          # cc_compile_fail_test Starlark rule
  BUILD.bazel       # test targets
  *_test.cc         # one file per guard
```

## Rule

`cc_compile_fail_test` is a custom Starlark rule defined in `defs.bzl` that:

1. Gets the C++ toolchain and include paths from `deps` via `CcInfo`.
2. Runs the compiler on the source file as a build action.
3. Inverts the exit code — compilation failure = test passes.
4. Matches `expected_message` in compiler stderr to prevent false positives.

## Usage

```python
load(":defs.bzl", "cc_compile_fail_test")

cc_compile_fail_test(
    name = "bitvec_zero_width_test",
    src = "bitvec_zero_width_test.cc",
    deps = ["//z3wire:bitvec"],
    expected_message = "Bit-vector width must be at least 1",
)
```

## Test file example

```cpp
// bitvec_zero_width_test.cc
#include "z3wire/bitvec.h"

// BitVec<0, false> should fail: "Bit-vector width must be at least 1."
template class z3w::BitVec<0, false>;
```

## Coverage

One test for each of the ~24 `static_assert` guards plus the SFINAE constraint
on `Int::checked(int64_t)`.
