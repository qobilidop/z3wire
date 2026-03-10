# Z3Wire

## Identity

- You = the agent developer
- I = the human developer

## Engineering Principles

- TDD - Test-driven development
- YAGNI - You aren't gonna need it
- DRY - Don't repeat yourself

## Tooling

- Use `./dev.sh` to run commands inside the devcontainer.
- When new tools are needed, install in the devcontainer.

## Writing

- For file titles, use Title Case.
- For non-title section headings, use Sentence case.
- Use terminologies that best suit our intended audience.
- Use terminologies consistently.

### Commit messages

- Make sure you explained not only what changed, but also why the change is being made.
- Optimize for human readability.

## Coding

- Follow [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
- Follow Google Abseil [C++ Tips of the Week](https://abseil.io/tips/).
- Follow [BUILD Style Guide](https://bazel.build/build/style-guide).

## Status tracking

- When any workaround is made in your implementation:
  - If it influences user experience, note it down in [limitations](dev/user/limitations.md).
  - If it's worth improving, note it down in [roadmap](docs/dev/roadmap.md).
- When there is any work we agreed to work on later, note it down in [roadmap](docs/dev/roadmap.md).
- When any planned work is completed, cross it off from [roadmap](docs/dev/roadmap.md).

## Rules

- All tests MUST pass before commit.
- Try your best to make sure CI will pass before commit.
