# Z3Wire — Agent Guide

## Repository map

```
docs/user/                    User guide (types, operations, casting, etc.).
docs/dev/design.md            Full design document. Read this first.
docs/dev/roadmap.md           Future directions and planned features.
docs/dev/plans/               Implementation plans.
z3wire/                       Library source: headers, sources, and tests.
examples/                     Runnable examples.
tools/format.sh               Auto-format (or --check) all C++ files.
tools/lint.sh                 Run clang-tidy static analysis (used in CI).
tools/coverage.sh             Generate LCOV coverage report (used in CI).
tools/docs.sh                 Build or serve the MkDocs documentation site.
dev.sh                        Run commands in the devcontainer.
.devcontainer/                Devcontainer setup (Ubuntu 24.04 + clang + Bazel).
.github/workflows/checks.yml  Checks workflow (format, lint).
.github/workflows/bazel.yml  Bazel workflow (build, test, coverage).
.github/workflows/cmake.yml  CMake workflow (build, test).
.github/workflows/docs.yml   Docs workflow (MkDocs → GitHub Pages).
MODULE.bazel                  Bazel module definition.
CMakeLists.txt                Root CMake build file.
renovate.json                 Renovate dependency update config.
```

Unit tests live alongside the code they test (`foo.h` → `foo_test.cc`).

## Build and test

Use `./dev.sh` to run commands inside the devcontainer:

```sh
./dev.sh bazel build //...    # build everything
./dev.sh bazel test //...     # run all tests
./dev.sh ./tools/format.sh    # auto-format all C++ files
./dev.sh ./tools/format.sh --check  # check formatting
./dev.sh ./tools/lint.sh      # run clang-tidy static analysis
./dev.sh ./tools/coverage.sh  # generate coverage report
```

All builds are hermetic via Bazel. Do not install dependencies outside of
Bazel. Only clang is supported (GCC is untested).

## CI

CI runs on every push to `main` and on PRs via GitHub Actions.
Four workflows: Checks (format, lint), Bazel (build, test, coverage),
CMake (build, test), and Docs (MkDocs deploy). All use `./dev.sh` to run
commands inside the devcontainer (except CMake and Docs which run on bare Ubuntu).

When adding a new CI workflow or deployable service, add a corresponding
badge to `README.md`.

## Dependency updates

Renovate (via the GitHub App) automates dependency update PRs. Chosen over
Dependabot because Renovate supports Bazel `MODULE.bazel`, `.bazelversion`,
GitHub Actions, and pip — Dependabot lacks Bazel support. Configuration is in
`renovate.json`.

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
Formatting is enforced by `clang-format` (Google style). Run `./tools/format.sh`
before completing a task.

Key conventions for this project:
- Types: `CamelCase` (`Bool`, `Ubv`, `Sbv`)
- Functions: `snake_case` (`to_bool`, `to_ubv1`, `checked_cast`)
- Template library: templates live in headers, non-template utilities go in
  `.cc` files.
- C++20 features: `static_assert`, `if constexpr`, `requires` clauses.

## Headings

Use sentence case for all headings — capitalize only the first word and proper
nouns (e.g., "Getting started", not "Getting Started").

## Terminology and spelling

- **bit-vector** (hyphenated) in prose — follows SMT-LIB and academic convention.
- **bitvec** in code identifiers — e.g., `bitvec.h`.
  - **Ubv** = unsigned bit-vector.
  - **Sbv** = signed bit-vector.
- **bit-width** (hyphenated) in prose.
- **bit-growth** (hyphenated) in prose.

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

- **Read the design doc** (`docs/dev/design.md`) before implementing any feature.
- **Write tests** for every new feature. Do not implement without a test.
- **Run `bazel test //...`** before considering any task complete.
- **Do not wrap `z3::context` or `z3::solver`.** Users interact with these
  directly; Z3Wire wraps only expressions.
- **Keep limitations up to date.** When adding a feature that has known
  limitations, or removing a limitation, update `docs/user/limitations.md`.
- **Always confirm before committing.** Never auto-commit. Show the changes
  and wait for explicit approval. Once approved, push automatically.
