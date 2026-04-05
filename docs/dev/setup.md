# Setup

How to set up the development environment.

## Prerequisites

- [Docker](https://www.docker.com/)
- [Dev Container CLI](https://github.com/devcontainers/cli)
- (Optional) An IDE with dev container support, e.g.
    [VS Code](https://code.visualstudio.com/) with the
    [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

All other dependencies are installed in the dev container.

## Dev container

We use [dev containers](https://containers.dev/) for both local development and
CI.

No separate setup is needed. The `./dev.sh` script calls `devcontainer up`
automatically, which builds the container image on first run.

## First build and test

```sh
git clone https://github.com/qobilidop/z3wire.git
cd z3wire
./dev.sh bazel build //...
./dev.sh bazel test //...
```

If both commands succeed, your environment is ready.
