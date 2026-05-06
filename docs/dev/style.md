# Style

## C++

- Follow
    [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
- Follow Google Abseil [C++ Tips of the Week](https://abseil.io/tips/).
- **Naming deviations:**
    - Free functions and access methods (member methods that return state
        directly, e.g., `value()`, `expr()`) use `snake_case`. This aligns with Z3
        and other SMT/hardware libraries in the ecosystem, and reads more
        naturally for hardware primitives.
    - All other class methods use `PascalCase`, following Google C++ Style. This
        includes static factories (e.g., `Literal`, `From`, `TryFrom`,
        `FromLeBytes`, `True`, `False`) and non-accessor instance methods (e.g.,
        `ToLeBytes`, `ToBeBytes`).
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
