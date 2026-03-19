# Development Guide

This guide is for both humans and agents.

## Development environment

We use [dev container](https://containers.dev/) for both local development and
CI.

### Prerequisites

- [Docker](https://www.docker.com/)
- [Dev Container CLI](https://github.com/devcontainers/cli)
- (Optional) An IDE with dev container support, e.g.
    [VS Code](https://code.visualstudio.com/) with the
    [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
- All other dependencies are installed in the dev container.

### Dev container setup

No separate setup is needed. The `./dev.sh` script calls `devcontainer up`
automatically, which builds the container image on first run.

## Commands

The `./dev.sh` script runs commands inside the dev container from the host. Omit
it if you're already inside.

### Bazel

#### Build

```sh
./dev.sh bazel build //...
```

#### Test

```sh
./dev.sh bazel test //...
```

### CMake

#### Configure

```sh
./dev.sh cmake -B build
```

#### Build

```sh
./dev.sh cmake --build build
```

#### Test

```sh
./dev.sh ctest --test-dir build --output-on-failure
```

### Format

```sh
./dev.sh ./tools/format.sh
```

To check without modifying (used in CI):

```sh
./dev.sh ./tools/format.sh --check
```

### Lint

```sh
./dev.sh ./tools/lint.sh
```

### Docs

#### Build

```sh
./dev.sh ./tools/docs.sh
```

#### Serve

```sh
./dev.sh ./tools/docs.sh serve
```

## Code style

### C++

- Follow
    [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
- Follow Google Abseil [C++ Tips of the Week](https://abseil.io/tips/).
- **Naming deviation:** Free functions and member methods use `snake_case`
    (e.g., `to_symbolic`, `safe_cast`, `shl`, `value()`, `expr()`), not
    `PascalCase`. This aligns with Z3 and other SMT/hardware libraries in the
    ecosystem, and reads more naturally for hardware primitives. Static factory
    methods use `PascalCase` (e.g., `Literal`, `Checked`, `True`, `False`).

### Python

- Follow
    [Google Python Style Guide](https://google.github.io/styleguide/pyguide.html).

### Bazel

- Follow [BUILD Style Guide](https://bazel.build/build/style-guide).

## Doc style

- Follow
    [Google Markdown Style Guide](https://google.github.io/styleguide/docguide/style.html).
- For titles, use Title Case.
- For non-title section headings, use Sentence case.
- Use plain hyphens (-) instead of em dashes.

## Quality checks

Before commit, ALWAYS run the following commands and fix any issues you see:

```sh
./dev.sh bazel build //...
./dev.sh bazel test //...

./dev.sh ./tools/lint.sh
./dev.sh ./tools/docs.sh

# Run format last so no subsequent edits undo it.
./dev.sh ./tools/format.sh
```

## Commit style

- Keep the subject line under 72 characters.
- Explain why the change is being made.
- Describe what has changed at a high level. Don't repeat what's obvious in the
    change itself.
- Prefer one commit per logical change. Don't split into too many tiny commits
    when they form a single unit of work.
- For agents: credit yourself (e.g. using `Co-authored-by:`) for commits you
    made.
