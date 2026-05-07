# Style

## C++

- Follow
    [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
- Follow Google Abseil [C++ Tips of the Week](https://abseil.io/tips/).
- **Naming:**
    - Free functions use `snake_case`, deviating from Google's `PascalCase`. This
        matches Z3 (the wrapped API) and `std::`, so calls like `z3::shl` and
        `z3w::shl` read consistently.
    - Class methods follow Google C++ Style: `PascalCase` for static factories
        (e.g., `Literal`, `From`, `TryFrom`, `True`, `False`) and non-accessor
        instance methods (e.g., `ToLeBytes`, `ToBeBytes`); `snake_case` for
        accessors that return state directly (e.g., `value()`, `expr()`).
    - Type traits use `snake_case` with a `_v` helper (e.g., `is_concrete`,
        `is_symbolic_v`), following `std::` conventions rather than Google's
        `PascalCase`.

## Bazel

- Follow [BUILD Style Guide](https://bazel.build/build/style-guide).

## Documentation

- Follow
    [Google Markdown Style Guide](https://google.github.io/styleguide/docguide/style.html).
- For titles, use Title Case.
- For non-title section headings, use Sentence case.
- Use plain hyphens (-) instead of em dashes.
