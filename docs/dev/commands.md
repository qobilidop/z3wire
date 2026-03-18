# Development Commands

The `./dev.sh` script runs commands inside the devcontainer. Omit it if you're
already inside.

## General checks

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

## Bazel

### Build

```sh
./dev.sh bazel build //...
```

### Test

```sh
./dev.sh bazel test //...
```

## CMake

### Configure

```sh
./dev.sh cmake -B build
```

### Build

```sh
./dev.sh cmake --build build
```

### Test

```sh
./dev.sh ctest --test-dir build --output-on-failure
```

## Docs

### Build

```sh
./dev.sh ./tools/docs.sh
```

### Serve

```sh
./dev.sh ./tools/docs.sh serve
```
