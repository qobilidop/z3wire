# Concrete Types

Z3Wire provides concrete fixed-width integer types that mirror the symbolic
`Ubv<W>` / `Sbv<W>` API. These are useful as standalone type-safe integers
and for mixing concrete values into symbolic expressions.

## Types

| Type | Description |
|:-----|:------------|
| `z3w::UInt<W>` | Unsigned concrete integer of width `W` (1--64) |
| `z3w::SInt<W>` | Signed concrete integer of width `W` (1--64) |

`UInt<W>` and `SInt<W>` are aliases for the underlying template:

```cpp
template <unsigned W, bool IsSigned>
class Int;

template <unsigned W> using UInt = Int<W, false>;
template <unsigned W> using SInt = Int<W, true>;
```

!!! info
Unlike symbolic types, concrete types do not require a `z3::context`.
They store plain native integers, not Z3 expressions.

## Construction

Three tiers, matching the symbolic API:

### Compile-time checked (Literal)

```cpp
auto x = z3w::UInt<8>::Literal<255>();   // OK
auto y = z3w::UInt<8>::Literal<256>();   // Compile error: out of range
auto z = z3w::SInt<8>::Literal<-128>();  // OK
auto w = z3w::SInt<8>::Literal<128>();   // Compile error: out of range
```

### Runtime checked

```cpp
auto [val, truncated] = z3w::UInt<8>::checked(300);
// val.value() == 44, truncated == true

auto [val2, truncated2] = z3w::UInt<8>::checked(200);
// val2.value() == 200, truncated2 == false
```

!!! note
`checked()` takes a `uint64_t` raw bit pattern. For signed types, pass
the two's complement representation, not a negative integer.

### Raw constructor

```cpp
z3w::UInt<8> x(300);  // Stores 300 & 0xFF = 44
```

The constructor is `explicit` to prevent accidental narrowing.

## Accessing the value

Use `.value()` to get the stored integer:

```cpp
z3w::UInt<8> x(42);
uint8_t v = x.value();  // 42
```

The return type is the smallest unsigned integer that fits the width
(`uint8_t` for W \<= 8, `uint16_t` for W \<= 16, etc.).

## Operations

All operations mirror the [symbolic API](operations.md), with two
differences:

1. **Comparison operators return `bool`**, not `z3w::Bool`.
1. **Checked operations return `bool` flags**, not symbolic `z3w::Bool`
   flags.

### Arithmetic (bit-growth)

```cpp
z3w::UInt<8> a(255), b(255);
auto sum = a + b;
// decltype(sum) is UInt<9>, sum.value() == 510

z3w::UInt<8> c(100), d(200);
auto diff = c - d;
// decltype(diff) is SInt<9> (subtraction is always signed)
```

Width and signedness rules are identical to the symbolic API.

### Bitwise, equality, comparison

Same operators as symbolic (`&`, `|`, `^`, `~`, `==`, `!=`, `<`, `<=`, `>`,
`>=`). Require matching width and signedness. Signed types use signed
comparison.

### Shifts, casting, bit manipulation

All three tiers are supported:

- **Shifts:** `<<`, `>>`, `checked_shl`, `checked_shr`, `lossless_shl`
- **Casting:** `cast`, `safe_cast`, `checked_cast`
- **Bit manipulation:** `extract`, `concat`
- **Conversion:** `to_uint1(bool)`, `to_bool(UInt<1>)`
- **Conditional:** `ite(bool, a, b)`

## Mixing with symbolic types

Concrete values automatically promote to symbolic when used in expressions
with symbolic operands:

```cpp
z3::context ctx;
z3w::Ubv<8> sym(ctx, "x");
z3w::UInt<8> conc(42);

auto result = sym + conc;  // Ubv<9> — concrete promoted automatically
z3w::Bool eq = (sym == conc);  // Symbolic equality check
```

The promotion grabs the `z3::context` from the symbolic operand, so no
context needs to be passed explicitly.

!!! note
This is the one exception to the "explicit over implicit" rule: concrete-
to-symbolic promotion is always lossless and unambiguous (same width,
same signedness), so it is allowed implicitly.

### Mixed ite

`ite` works with any combination of symbolic and concrete arguments:

```cpp
z3w::Bool cond(ctx, "c");
z3w::UInt<8> a(42);
z3w::UInt<8> b(99);

auto result = z3w::ite(cond, a, b);  // Promotes both to symbolic
```

### Explicit promotion

To convert a concrete value to symbolic manually:

```cpp
z3w::UInt<8> conc(42);
z3w::Ubv<8> sym = z3w::to_symbolic(conc, ctx);
```

## Storage

Both `UInt` and `SInt` use unsigned storage internally. For `SInt`, values
are stored in two's complement representation.

!!! info "Why unsigned storage?"
Signed integer overflow is undefined behavior in C++, while unsigned
overflow is well-defined (wraps mod 2^N). A bit-vector library performs
intermediate arithmetic with masking, and unsigned storage avoids UB.
The bits are the same either way; signed interpretation happens only at
comparison and sign-extension boundaries.
