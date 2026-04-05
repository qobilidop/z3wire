# Workflow

## Test-driven development

We follow TDD. Write the failing test first, then the minimal implementation to
make it pass.

## Quality checks

Before commit, ALWAYS run the following commands and fix any issues:

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
