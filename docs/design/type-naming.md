# Type Naming

## Motivation

Z3Wire's type system has two tiers: symbolic types (Z3 solver domain) and
concrete types (C++ runtime domain). The original naming used Z3-flavored names
for symbolic types (`Bool`, `Ubv<W>`, `Sbv<W>`) and C++-flavored names for
concrete types (`UInt<W>`, `SInt<W>`, native `bool`). This had two problems:

1. **No concrete `Bool` type.** Native `bool` accepts implicit conversions from
    integers, pointers, and many other types, undermining Z3Wire's "explicit over
    implicit" philosophy.

1. **Inconsistent naming.** The symbolic/concrete distinction was encoded through
    naming convention (Z3-flavored vs C++-flavored) rather than a consistent
    prefix. `Bool` reads as a natural C++ name, not obviously Z3-specific.

This design supersedes the statement in `overview.md` that "Bool does not need a
concrete counterpart." That decision assumed native `bool` was sufficient, but
the implicit conversion problem motivates a type-safe wrapper.

## Design

### Naming scheme

All symbolic types get a `Sym` prefix. Concrete types get the natural C++ names.

| Symbolic                     | Concrete                  |
| ---------------------------- | ------------------------- |
| `SymBool`                    | `Bool`                    |
| `SymUInt<W>`                 | `UInt<W>`                 |
| `SymSInt<W>`                 | `SInt<W>`                 |
| `SymBitVec<W, S>` (template) | `BitVec<W, S>` (template) |

The `Sym` prefix is self-documenting: no domain knowledge is needed to
understand which tier a type belongs to.

### Header files

| Before                                         | After                                                          |
| ---------------------------------------------- | -------------------------------------------------------------- |
| `bool.h` (`Bool` class)                        | `sym_bool.h` (`SymBool` class)                                 |
| `bitvec.h` (`BitVec<W,S>`, `Ubv<W>`, `Sbv<W>`) | `sym_bit_vec.h` (`SymBitVec<W,S>`, `SymUInt<W>`, `SymSInt<W>`) |
| `int.h` (`Int<W,S>`, `UInt<W>`, `SInt<W>`)     | `bit_vec.h` (`BitVec<W,S>`, `UInt<W>`, `SInt<W>`)              |
| (none)                                         | `bool.h` (new concrete `Bool` class)                           |

### Concrete `Bool`

A type-safe wrapper around native `bool` that prevents implicit construction
from non-boolean types.

```cpp
class Bool {
 public:
  constexpr Bool() : value_(false) {}
  constexpr Bool(bool v);                          // implicit from bool literals
  template <std::integral T> Bool(T) = delete;     // blocks int/char/etc.

  explicit constexpr operator bool() const;
  constexpr bool value() const;

  friend constexpr bool operator==(Bool lhs, Bool rhs);
  friend constexpr bool operator!=(Bool lhs, Bool rhs);
  friend constexpr bool operator==(Bool lhs, bool rhs);
  friend constexpr bool operator==(bool lhs, Bool rhs);

  friend std::ostream& operator<<(std::ostream& os, Bool b);
  // Output format: "true" or "false".

 private:
  bool value_;
};
```

Design choices:

- **`constexpr` throughout**: consistent with concrete `BitVec<W,S>`.
- **Default-initializes to `false`**: avoids uninitialized state, consistent
    with `BitVec` zero-initializing its `bits_` member.
- **Implicit from `bool`**: `Bool b = true;` works naturally.
- **Deleted integral constructor**: `Bool b = 42;` is a compile error.
- **`explicit operator bool()`**: prevents accidental use in arithmetic
    contexts. C++ contextual conversion still allows `if (b)`, `!b`, `b && c`,
    and `b ? x : y` without a cast.
- **`operator==` with `bool`**: `EXPECT_EQ(b, true)` works without casting.
- **`value()` accessor**: named alternative to `operator bool()`.

Type trait integration:

- `is_concrete<Bool>` is `true`.

Mixed `Bool`/`SymBool` operands are not supported via `mixed_operands` because
concrete `Bool` carries no Z3 context. Promotion to `SymBool` requires an
explicit context, following the same pattern as `SymBool::True(ctx)` and
`SymBool::False(ctx)`. A `to_symbolic(Bool b, z3::context& ctx)` free function
provides this.

### Transition strategy

One-shot rename with no backward compatibility aliases. Z3Wire is pre-1.0 with
no external consumers, so a clean break is simplest.

## Scope

| Category                    | Count |
| --------------------------- | ----- |
| Core headers modified       | 3     |
| New header                  | 1     |
| Test files renamed/updated  | ~23   |
| Example files updated       | ~5    |
| Codegen files updated       | ~3    |
| Documentation files updated | ~14   |
