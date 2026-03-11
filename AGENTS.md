# Z3Wire

## Roles

- You (the agent) = the developer
- I (the human) = the project lead

## Engineering principles

- TDD - Test-driven development
- YAGNI - You aren't gonna need it
- DRY - Don't repeat yourself
- Local reasoning - Each piece should be understandable on its own
- Flat structure - Prefer shallow directory hierarchies; don't nest without a reason

## Tooling

- Run ALL commands through `./dev.sh`. Never run build, test, or install commands directly on the host.
- When new tools are needed, install in the devcontainer.

## Writing

- Project name: "Z3Wire" in prose, `z3wire` in code/paths/URLs.
- For file titles, use Title Case.
- For non-title section headings, use Sentence case.
- Use terminologies that best suit our intended audience.
- Use terminologies consistently.

### Commit messages

- Keep the subject line under 72 characters.
- Make sure you explain not only what changed, but also why the change is being made.
- Optimize for human readability.

## Coding

- Follow [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html),
    with the following deliberate deviations:
    - **Functions use `snake_case`** (not `CamelCase`). This matches the Z3 C++
        API that users call alongside Z3Wire, keeping mixed code readable. Static
        factory methods (`True()`, `False()`, `Literal()`) use `CamelCase` as they
        act as named constructors.
    - **Type traits follow STL convention**: `is_symbolic`, `is_concrete`,
        `is_symbolic_v`, `is_concrete_v` (not `IsSymbolic`, etc.).
- Follow Google Abseil [C++ Tips of the Week](https://abseil.io/tips/).
- Follow [BUILD Style Guide](https://bazel.build/build/style-guide).
- Tests live next to the code they test: `foo.h` → `foo_test.cc` in the same
    directory. Compile-fail tests go in `compile_fail_tests/`.

## Documentation

- Docs use mkdocs Material theme. Run `./dev.sh ./tools/docs.sh build` to
    verify docs build before committing doc changes.
- New features should have a design doc in `docs/design/`.
- Update `docs/usage/cheatsheet.md` when adding new API surface.
- Don't place non-doc files (plans, specs) inside `docs/` — mkdocs strict mode
    will reject broken links in any file under that directory.

## Status tracking

- When any workaround is made in your implementation:
    - If it influences user experience, note it down in [limitations](docs/usage/limitations.md).
    - If it's worth improving, note it down in [roadmap](docs/dev/roadmap.md).
- When there is any work we agreed to work on later, note it down in [roadmap](docs/dev/roadmap.md).
- When any planned work is completed, cross it off from [roadmap](docs/dev/roadmap.md).

## Rules

- Run `./dev.sh ./tools/format.sh` before every commit.
- Run `./dev.sh ./tools/docs.sh build` before committing doc changes.
- Run `./dev.sh ./tools/lint.sh` to catch clang-tidy and shellcheck issues
    (not required before every commit, but fix any issues before pushing).
- All tests MUST pass before commit.
- Try your best to make sure CI will pass before commit.
- Always confirm with me before committing. Auto-push after commit.
