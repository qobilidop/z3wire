# CLAUDE.md

## What is Z3Wire?

A type-safe, header-only C++20 abstraction layer for Z3 that enforces
compile-time bit-width consistency and precision-preserving "natural growth"
arithmetic for hardware modeling and formal verification.

- **Namespace:** `z3w::`
- **Scope:** Strictly Bit-Vectors and Booleans (QF_BV theory).
- **Design doc:** [docs/design.md](docs/design.md)

## Build

- C++20
- Bazel (primary build system)
- CMake (secondary)

## Coding convention

- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
