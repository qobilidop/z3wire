# Z3Wire

## Engineering principles

- **TDD**: Test-Driven Development
- **YAGNI**: You Aren't Gonna Need It
- **DAMP**: Descriptive and Meaningful Phrases
- **DRY**: Don't Repeat Yourself
- Local reasoning

## Development environment

- Assume devcontainer as our only development environment and use it consistently.
- Install ALL dependencies in devcontainer.
- Run ALL [development commands](docs/dev/commands.md) through `./dev.sh`.

## Doc style

- Optimize for human readability.
- For titles, use Title Case.
- For non-title section headings, use Sentence case.
- Use plain hyphens (-) instead of em dashes.
- Use terminologies consistently.

## Code style

- [C++] Follow [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
- [C++] Follow Google Abseil [C++ Tips of the Week](https://abseil.io/tips/).
- [Python] Follow [Google Python Style Guide](https://google.github.io/styleguide/pyguide.html).
- [Bazel] Follow [BUILD Style Guide](https://bazel.build/build/style-guide).

## Quality checks

- Run `./dev.sh ./tools/format.sh` before commit.
- Run `./dev.sh ./tools/lint.sh` before commit and fix all issues.
- Run `./dev bazel test //...` before commit and fix all issues.
- Run `./dev.sh ./tools/docs.sh` before commit and fix all issues.

## Commit style

- Keep the subject line under 72 characters.
- Explain why the change is being made.
- Describe what has changed at a high level. Don't repeat what's obvious in the change itself.
- Prefer one commit per logical change. Don't split into many tiny commits when they form a single unit of work.
- Credit yourself (e.g. using the `Co-authored-by:` field) for commits you made.

## Rules

- All tests MUST pass before commit.
- Try your best to make sure CI will pass before commit.
