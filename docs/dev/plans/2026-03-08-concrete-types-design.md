# Concrete bit-vector types

## Motivation

Z3Wire provides symbolic bit-vector types (`Ubv<W>`, `Sbv<W>`) that wrap Z3
expressions. However, there is no type-safe way to represent concrete
fixed-width integers in C++ — native types like `uint8_t` only cover
power-of-two widths and lack bit-growth semantics.

Concrete types serve two purposes:

1. **Mixed concrete/symbolic expressions.** Users can write `symbolic_x + concrete_y`
   without manually creating Z3 constants.
2. **Standalone type-safe integers.** `UInt<5>` or `SInt<12>` are useful even
   without Z3, enforcing bit-width at the type level.

`Bool` does not need a concrete counterpart — the native C++ `bool` is
sufficient.

## Types and storage

Two new type aliases, mirroring the symbolic `Ubv<W>` / `Sbv<W>` pattern:

```cpp
template <unsigned W, bool IsSigned>
class Int;

template <unsigned W> using UInt = Int<W, false>;
template <unsigned W> using SInt = Int<W, true>;
```

**Width constraint:** `static_assert(W >= 1 && W <= 64)`.

**Storage type:** the smallest unsigned standard integer that fits:

```cpp
template <unsigned W>
using StorageType = std::conditional_t<(W <= 8), uint8_t,
                    std::conditional_t<(W <= 16), uint16_t,
                    std::conditional_t<(W <= 32), uint32_t, uint64_t>>>;
```

Both `UInt` and `SInt` use unsigned storage. Values are stored masked to W
bits; for `SInt`, the stored bits are the two's complement representation and
sign extension happens at interpretation time.

**Why unsigned storage for `SInt`?** Signed integer overflow is undefined
behavior in C++, while unsigned overflow is well-defined (wraps mod 2^N). A
bit-vector library performs masking and wrapping arithmetic on intermediate
values, and unsigned storage avoids UB traps. The bits are identical either
way (two's complement); we reinterpret as signed only at boundaries where it
matters (comparisons, sign extension for casts). For non-standard widths like
`SInt<5>` stored in `uint8_t`, manual sign extension from bit 4 is required
regardless of storage signedness, so signed types offer no advantage.

## Construction

Three tiers, matching the library's existing pattern:

**Compile-time checked (`Literal`):**

```cpp
auto x = UInt<8>::Literal<255>();   // OK
auto y = UInt<8>::Literal<256>();   // Compile error: out of range
auto z = SInt<8>::Literal<-128>();  // OK
auto w = SInt<8>::Literal<128>();   // Compile error: out of range
```

No `z3::context` needed (unlike the symbolic `Ubv<8>::Literal<255>(ctx)`).

**Runtime checked (`checked`):**

```cpp
auto [val, truncated] = UInt<8>::checked(300);
// val is UInt<8> holding 44, truncated is true

auto [val2, truncated2] = UInt<8>::checked(200);
// val2 is UInt<8> holding 200, truncated2 is false
```

Returns a `{value, truncated}` pair. The flag is a plain `bool`.

**Raw constructor (masks silently):**

```cpp
UInt<8> x(300);  // Stores 300 & 0xFF = 44
```

The constructor is `explicit` to prevent accidental narrowing from plain
integers.

## Operations

All operations mirror the symbolic API, operating on native integers with
masking to enforce bit-width. All flags (`truncated`, `overflowed`, `lost`)
are plain `bool`, not symbolic `Bool`.

**Arithmetic (with bit-growth):**

- `UInt<W1> + UInt<W2>` yields `UInt<max(W1,W2)+1>`
- `UInt<W1> - UInt<W2>` yields `SInt<max(W1,W2)+1>` (subtraction always signed)
- Signed if either operand is signed (same rules as symbolic)

**Bitwise (strict width/signedness matching):**

- `&`, `|`, `^`, `~`

**Comparisons:**

- `==`, `!=`, `<`, `<=`, `>`, `>=` — return plain `bool`
- Signed types use signed comparison, unsigned use unsigned

**Shifts (three tiers):**

- Hardware shifts (`<<`, `>>`) — fixed width, bits lost silently
- `checked_shl` / `checked_shr` — returns `{result, lost_bits}`
- `lossless_shl` — widens the result

**Bit manipulation:**

- `extract<High, Low>(val)` — static bit extraction
- `concat(a, b, ...)` — concatenation, returns `UInt`
- `to_uint1(bool)` / `to_bool(UInt<1>)` — `bool` / `UInt<1>` conversion

**Casting (three tiers):**

- `cast<T>(val)` — raw
- `safe_cast<T>(val)` — compile-time lossless check
- `checked_cast<T>(val)` — returns `{result, overflowed}` with plain `bool`

**Conditional:**

- `ite(bool, UInt<W>, UInt<W>)` — returns concrete type

## Promotion to symbolic

Concrete values implicitly promote to symbolic when mixed in expressions
with symbolic operands. The Z3 context is obtained from the symbolic
operand's `z3::expr`.

```cpp
Ubv<8> symbolic_x(ctx, "x");
UInt<8> concrete_y(42);
auto result = symbolic_x + concrete_y;  // result is Ubv<9>
```

**Overload strategy:** C++20 concepts keep the overload count minimal.
Instead of separate overloads for every (symbolic, concrete) and (concrete,
symbolic) combination, a single constrained template per operator handles
all mixed cases:

```cpp
template <typename Cond, typename T, typename F>
  requires (is_symbolic_v<Cond> || is_symbolic_v<T> || is_symbolic_v<F>)
auto ite(Cond cond, T true_val, F false_val);
```

This reduces `ite` from 8 potential overloads to 2 (pure concrete + one
constrained template for anything involving a symbolic argument).

**Context extraction:** a compile-time `if constexpr` chain finds the first
symbolic argument and grabs its context. All symbolic values in a valid
program share the same `z3::context`, so it does not matter which one is
chosen.

```cpp
if constexpr (is_symbolic_v<Cond>) {
  auto& ctx = cond.raw().ctx();
} else if constexpr (is_symbolic_v<T>) {
  auto& ctx = true_val.raw().ctx();
} else {
  auto& ctx = false_val.raw().ctx();
}
```

## File organization

- `z3wire/int.h` — `Int<W, IsSigned>`, `UInt<W>`, `SInt<W>` and all
  concrete-concrete operations.
- `z3wire/int_test.cc` — tests for pure concrete operations.
- `z3wire/bitvec.h` — mixed (concrete + symbolic) overloads, added alongside
  existing symbolic operators. Includes `z3wire/int.h`.
- `z3wire/bitvec_test.cc` — tests for mixed operations.
