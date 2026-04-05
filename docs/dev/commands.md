# Commands

The `./dev.sh` script runs commands inside the dev container from the host. Omit
it if you're already inside the container.

## Bazel (primary build system)

### Build

```sh
./dev.sh bazel build //...
```

### Test

```sh
./dev.sh bazel test //...
```

## CMake (secondary build system)

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

## Format

```sh
./dev.sh ./tools/format.sh
```

To check without modifying (used in CI):

```sh
./dev.sh ./tools/format.sh --check
```

## Lint

```sh
./dev.sh ./tools/lint.sh
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
