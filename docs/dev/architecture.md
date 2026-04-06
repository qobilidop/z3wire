# Architecture

A map of the Z3Wire codebase.

## Directory layout

```
z3wire/              Library source - headers and tests
  *.h                Public API headers
  *_test.cc          Unit tests (colocated with headers)
  BUILD.bazel        Bazel targets for library and tests
  CMakeLists.txt     CMake build config

examples/            Standalone example programs
  usage/             Runnable examples linked from usage docs

tests/
  compile_fail/      Compile-time type safety tests
    defs.bzl         Custom cc_compile_fail_test rule
  fuzz/              Fuzz tests

docs/                Zensical documentation source
  design/            Design decision records
  usage/             API usage guides
  dev/               Development guides (this directory)

tools/               Dev scripts (format, lint, docs)
.devcontainer/       Dev container configuration
.github/             CI workflows
```

## Type hierarchy

All types live in namespace `z3w::`.

**Symbolic types** (wrap `z3::expr`, require `z3::context`):

- `SymBool` - symbolic boolean
- `SymBitVec<W, IsSigned>` - symbolic bit-vector (base template)
    - `SymUInt<W>` = `SymBitVec<W, false>` - unsigned alias
    - `SymSInt<W>` = `SymBitVec<W, true>` - signed alias

**Concrete types** (native values, no Z3 dependency):

- `Bool` - concrete boolean (only accepts `bool`)
- `BitVec<W, IsSigned>` - concrete bit-vector (base template)
    - `UInt<W>` = `BitVec<W, false>` - unsigned alias
    - `SInt<W>` = `BitVec<W, true>` - signed alias
    - W \<= 64: backed by native integer
    - W > 64: backed by byte array

## Build targets

### Bazel (primary)

Each header has its own `cc_library` target and a corresponding `cc_test`:

| Library                | Header          | Test               | Dependencies                             |
| ---------------------- | --------------- | ------------------ | ---------------------------------------- |
| `//z3wire:bool`        | `bool.h`        | `bool_test`        | (none)                                   |
| `//z3wire:bit_vec`     | `bit_vec.h`     | `bit_vec_test`     | `type_traits`                            |
| `//z3wire:sym_bool`    | `sym_bool.h`    | `sym_bool_test`    | `bool`, `check`, Z3                      |
| `//z3wire:sym_bit_vec` | `sym_bit_vec.h` | `sym_bit_vec_test` | `bit_vec`, `sym_bool`, `type_traits`, Z3 |
| `//z3wire:type_traits` | `type_traits.h` | (none)             | (none)                                   |
| `//z3wire:check`       | `check.h`       | `check_test`       | (none, internal visibility)              |

Compile-fail tests use a custom rule in `tests/compile_fail/defs.bzl`.

### CMake (secondary)

CMake mirrors the Bazel structure. Both build systems pin the same Z3 version
(in `MODULE.bazel` and the dev container Dockerfile respectively).

## Test organization

- **Unit tests** (`z3wire/*_test.cc`): colocated with headers, one test file per
    header. Use Google Test.
- **Compile-fail tests** (`tests/compile_fail/*_test.cc`): one file per
    `static_assert` guard. Verify that invalid code fails to compile with the
    expected error message.
- **Fuzz tests** (`tests/fuzz/`): property-based tests using Google FuzzTest.
