# Cast Rename

Rename the symbolic casting API to make safety the default and hardware
semantics explicitly opt-in.

## Motivation

Z3Wire's selling point is preventing bad things from happening implicitly.
The current naming puts the raw hardware cast behind the shortest, most
natural name (`cast`), which contradicts this philosophy. The safe tier
should be the easiest to reach for, and the unsafe tier should be called
out explicitly.

## Current API

| Name              | Semantics                           |
| :---------------- | :---------------------------------- |
| `cast<T>`         | Raw HW truncation/extension/bitcast |
| `safe_cast<T>`    | Compile error if lossy              |
| `checked_cast<T>` | Returns `{result, value_preserved}` |

## New API

| Name              | Semantics                           | Who verifies |
| :---------------- | :---------------------------------- | :----------- |
| `safe_cast<T>`    | Compile error if lossy              | Compiler     |
| `checked_cast<T>` | Returns `{result, value_preserved}` | Solver       |
| `unsafe_cast<T>`  | Raw HW truncation/extension/bitcast | You          |

The `safe` / `checked` / `unsafe` prefixes form a spectrum of verification
strength, each name self-documenting its contract.

## Naming rationale

- **`safe_cast`** - Explicit about what it guarantees. Consistent with
    Z3Wire's "explicit over implicit" principle. Self-documenting when read
    in isolation (local reasoning). No surprise for C++ developers, who
    associate bare `cast` with "just do it."
- **`checked_cast`** - Unchanged. Matches the Rust/C++ convention where
    `checked_*` means "do it and report success/failure." Accurate for the
    return-a-flag semantics.
- **`unsafe_cast`** - Parallels Rust's `unsafe`: not "this is wrong" but
    "the compiler can't verify this for you." Appropriate for Z3Wire's
    formal verification audience, who understand the verified/unverified
    distinction.

## Scope

This spec covers the three casting functions only:

- `cast` -> `unsafe_cast`
- `safe_cast` -> `safe_cast` (unchanged)
- `checked_cast` -> `checked_cast` (unchanged)

Shift operations (`shl`, `checked_shl`, `lossless_shl`) will be addressed
separately.

## Changes required

### Header (`z3wire/sym_bit_vec.h`)

- Rename `cast` to `unsafe_cast`.
- Update `safe_cast` body (calls `cast` internally, update to `unsafe_cast`).
- Update `checked_cast` body (calls `cast` internally, update to `unsafe_cast`).
- Update internal callers (`lossless_shl` variants call `cast`).

### Code generator (`z3wire/weave/emit_header.py`)

- Update emitted `z3w::cast<...>` calls to `z3w::unsafe_cast<...>`.
- Regenerate `examples/weave/status_register.expected.h` from the updated
    generator.

### Tests (`z3wire/sym_bit_vec_test.cc`)

- Rename all `cast<T>(...)` calls to `unsafe_cast<T>(...)` in cast tests.

### Compile-fail tests (`compile_fail_tests/`)

- No changes needed. Confirmed these only reference `safe_cast`.

### Examples

- `examples/alu.cc` - Uses `cast` and `checked_cast`.
- `examples/safe_adder.cc` - Uses `checked_cast` only, no changes needed.

### Documentation

- `README.md` - Casting tier description.
- `docs/usage/types.md` - Casting section.
- `docs/usage/cheatsheet.md` - Casting table.
- `docs/usage/operations.md` - Tip referencing `cast`.
- `docs/design/overview.md` - Type conversions section.
- `docs/examples/alu.md` - Code blocks with `cast`.
- `docs/dev/test-coverage.md` - Section headers referencing `cast`.
