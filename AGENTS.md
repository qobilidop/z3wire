# Z3Wire — Agent Guide

## Repository map

```
docs/design.md               Full design document. Read this first.
include/z3wire/               Public headers (TODO: not yet created).
tests/                        Tests (TODO: not yet created).
tools/ci_smoke_test.sh        CI smoke test.
.devcontainer/                Devcontainer setup (Ubuntu 24.04 + clang + Bazel).
.github/workflows/ci.yml     CI workflow.
BUILD.bazel                   Root build file.
MODULE.bazel                  Bazel module definition.
```

Unit tests should live alongside the code they test or in a `tests/` directory.

## Build and test

```sh
bazel build //...             # build everything
bazel test //...              # run all tests
```

All builds are hermetic via Bazel. Do not install dependencies outside of
Bazel.

## CI

CI runs on every push to `main` and on PRs via GitHub Actions
(`.github/workflows/ci.yml`). It builds the devcontainer image and runs
`bazel build //...` and `bazel test //...` inside it.

## Key design invariants — do not break these

1. **Compile-time type safety.** Bit-width and signedness mismatches must be
   compile errors, never runtime errors. Do not add implicit conversions.

2. **Bit-growth arithmetic.** `+` and `-` must widen the result type to
   `max(W1, W2) + 1`. Do not silently truncate arithmetic results.

3. **Explicit over implicit.** No implicit conversions between signed/unsigned
   or different widths. Users must use the casting API (`cast`, `safe_cast`,
   `checked_cast`) to express intent.

4. **Zero overhead.** Each wrapper stores only a `z3::expr`. No virtual
   functions, no extra data members.

5. **Scope: Booleans and fixed-width bit-vectors only.** Do not add support
   for unbounded integers, reals, arrays, floating-point, or uninterpreted
   functions.

## Style

Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

Key conventions for this project:
- Types: `CamelCase` (`Ubv`, `Sbv`, `Bool`, `BitVec`)
- Functions: `snake_case` (`to_bool`, `to_ubv1`, `checked_cast`)
- Template library: templates live in headers, non-template utilities go in
  `.cc` files.
- C++20 features: `static_assert`, `if constexpr`, `requires` clauses.

## Commit messages

Focus on *why* the change is being made. Reference the design doc or issue
where applicable. Keep it concise.

## Pull requests

Open PRs against `main`. Lead with a summary of what changed and why.

## Worktrees

When working on independent tasks in parallel, use dedicated git worktrees:

```sh
git worktree add ../z3wire-<branch> -b <branch>
```

## Expectations

- **Read the design doc** (`docs/design.md`) before implementing any feature.
- **Write tests** for every new feature. Do not implement without a test.
- **Run `bazel test //...`** before considering any task complete.
- **Do not wrap `z3::context` or `z3::solver`.** Users interact with these
  directly; Z3Wire wraps only expressions.
