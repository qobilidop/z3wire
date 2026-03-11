# Types

Z3Wire provides symbolic types that wrap Z3 expressions, concrete types that
store plain values, and a three-tier casting API for converting between them.

## Symbolic types

Zero-overhead wrappers around `z3::expr`.

| Type          | Description                      |
| :------------ | :------------------------------- |
| `z3w::Bool`   | Symbolic boolean                 |
| `z3w::Ubv<W>` | Unsigned bit-vector of width `W` |
| `z3w::Sbv<W>` | Signed bit-vector of width `W`   |

`Ubv<W>` and `Sbv<W>` are aliases for the underlying template:

```cpp
template <size_t Width, bool IsSigned>
class BitVec;

template <size_t W> using Ubv = BitVec<W, false>;
template <size_t W> using Sbv = BitVec<W, true>;
```

!!! info

    `BitVec<0, S>` is forbidden via `static_assert`. A zero-width bit-vector has
    no meaning in hardware or SMT.

### Creating symbolic variables

All types are constructed with a Z3 context and a name:

```cpp
z3::context ctx;

z3w::Bool flag(ctx, "flag");
z3w::Ubv<32> address(ctx, "address");
z3w::Sbv<16> offset(ctx, "offset");
```

### Literals

Bool literals:

```cpp
z3w::Bool t = z3w::Bool::True(ctx);
z3w::Bool f = z3w::Bool::False(ctx);
```

Bit-vector literals are range-checked at compile time:

```cpp
auto x = z3w::Ubv<8>::Literal<255>(ctx);  // OK
auto y = z3w::Ubv<8>::Literal<256>(ctx);  // Compile error: value doesn't fit
```

### Accessing the underlying Z3 expression

Use `.raw()` to get the underlying `z3::expr` for interop with Z3 APIs:

```cpp
z3w::Ubv<8> x(ctx, "x");
solver.add(x.raw() > 0);  // Pass to Z3 solver directly
```

!!! note

    Z3Wire does not wrap `z3::context` or `z3::solver`. You interact with these
    directly.

## Concrete types

Fixed-width integer types that mirror the symbolic API. Useful as standalone
type-safe integers and for mixing concrete values into symbolic expressions.

| Type           | Description                                    |
| :------------- | :--------------------------------------------- |
| `z3w::UInt<W>` | Unsigned concrete integer of width `W` (1--64) |
| `z3w::SInt<W>` | Signed concrete integer of width `W` (1--64)   |

`UInt<W>` and `SInt<W>` are aliases for the underlying template:

```cpp
template <size_t W, bool IsSigned>
class Int;

template <size_t W> using UInt = Int<W, false>;
template <size_t W> using SInt = Int<W, true>;
```

!!! info

    Unlike symbolic types, concrete types do not require a `z3::context`.
    They store plain native integers, not Z3 expressions.

### Construction

Three tiers, matching the symbolic API:

**Compile-time checked (Literal):**

```cpp
auto x = z3w::UInt<8>::Literal<255>();   // OK
auto y = z3w::UInt<8>::Literal<256>();   // Compile error: out of range
auto z = z3w::SInt<8>::Literal<-128>();  // OK
auto w = z3w::SInt<8>::Literal<128>();   // Compile error: out of range
```

**Runtime checked:**

```cpp
auto [val, truncated] = z3w::UInt<8>::checked(300);
// val.value() == 44, truncated == true

auto [val2, truncated2] = z3w::UInt<8>::checked(200);
// val2.value() == 200, truncated2 == false
```

!!! note

    `checked()` takes a `uint64_t` raw bit pattern. For signed types, pass
    the two's complement representation, not a negative integer.

**Raw constructor:**

```cpp
z3w::UInt<8> x(300);  // Stores 300 & 0xFF = 44
```

The constructor is `explicit` to prevent accidental narrowing.

### Accessing the value

Use `.value()` to get the stored integer:

```cpp
z3w::UInt<8> x(42);
uint8_t v = x.value();  // 42
```

The return type is the smallest unsigned integer that fits the width
(`uint8_t` for W \<= 8, `uint16_t` for W \<= 16, etc.).

### Mixing with symbolic types

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

To convert a concrete value to symbolic manually:

```cpp
z3w::UInt<8> conc(42);
z3w::Ubv<8> sym = z3w::to_symbolic(conc, ctx);
```

### Storage

Both `UInt` and `SInt` use unsigned storage internally. For `SInt`, values
are stored in two's complement representation.

!!! info "Why unsigned storage?"

    Signed integer overflow is undefined behavior in C++, while unsigned
    overflow is well-defined (wraps mod 2^N). A bit-vector library performs
    intermediate arithmetic with masking, and unsigned storage avoids UB.
    The bits are the same either way; signed interpretation happens only at
    comparison and sign-extension boundaries.

## Casting

Z3Wire provides a three-tier casting API, giving you explicit control over how
bit-vectors change width or signedness. The same API works for both symbolic
and concrete types.

### `cast<T>(val)` — Hardware cast

Raw truncation, extension, or sign reinterpretation. No safety checks. Use when
you intentionally want hardware-style overflow or wrap behavior.

```cpp
z3w::Ubv<16> wide(ctx, "wide");
auto narrow = z3w::cast<z3w::Ubv<8>>(wide);   // Truncate to 8 bits
auto wider = z3w::cast<z3w::Ubv<32>>(wide);    // Zero-extend to 32 bits
auto reint = z3w::cast<z3w::Sbv<16>>(wide);    // Reinterpret as signed
```

Under the hood:

| Conversion                  | Operation                                             |
| :-------------------------- | :---------------------------------------------------- |
| Target narrower than source | `z3::extract` (truncation)                            |
| Target wider than source    | `z3::zext` or `z3::sext` (based on source signedness) |
| Same width                  | Zero-overhead type reinterpretation                   |

### `safe_cast<T>(val)` — Compile-time guard

Only compiles if the cast is mathematically guaranteed to be lossless. Use when
you want the compiler to verify that no data can be lost.

```cpp
z3w::Ubv<8> small(ctx, "small");
auto wide = z3w::safe_cast<z3w::Ubv<16>>(small);   // OK: widening
auto bad = z3w::safe_cast<z3w::Ubv<4>>(small);      // Compile error!
```

**Rules:**

| Source    | Target    | Allowed?                                       |
| :-------- | :-------- | :--------------------------------------------- |
| `Ubv<W1>` | `Ubv<W2>` | Yes, if `W2 >= W1`                             |
| `Sbv<W1>` | `Sbv<W2>` | Yes, if `W2 >= W1`                             |
| `Ubv<W1>` | `Sbv<W2>` | Yes, if `W2 > W1` (needs 1 extra bit for sign) |
| `Sbv<W1>` | `Ubv<W2>` | **Always forbidden** (negative values corrupt) |
| Any       | Smaller   | **Always forbidden** (truncation is not safe)  |

### `checked_cast<T>(val)` — Verification cast

Performs the cast and returns a symbolic boolean indicating whether data loss
occurred. Use when you want to verify safety as part of a Z3 proof.

```cpp
z3w::Ubv<16> val(ctx, "val");
auto [result, overflowed] = z3w::checked_cast<z3w::Ubv<8>>(val);

// Assert in the solver: this cast must never lose data.
solver.add(!overflowed.raw());
```

The overflow flag works by round-tripping: cast to the target type, cast back,
and check if the value changed.

### Choosing the right tier

| Situation                                  | Use            |
| :----------------------------------------- | :------------- |
| Modeling hardware truncation (intentional) | `cast`         |
| Widening where data loss is impossible     | `safe_cast`    |
| Need to prove no data loss symbolically    | `checked_cast` |

## Bool / bit-vector conversion

In Z3, `Bool` and a 1-bit bit-vector are distinct sorts. Hardware frequently
needs to convert between them. Z3Wire provides explicit conversion functions:

```cpp
// Bit-vector to Bool
z3w::Ubv<32> status(ctx, "status");
z3w::Bool ready = z3w::to_bool(z3w::extract<0, 0>(status));

// Bool to bit-vector
z3w::Bool cond(ctx, "cond");
z3w::Ubv<1> flag = z3w::to_ubv1(cond);
```

Concrete equivalents:

```cpp
z3w::UInt<1> one = z3w::to_uint1(true);   // UInt<1>(1)
bool b = z3w::to_bool(z3w::UInt<1>(1));   // true
```

## Design principle: explicit over implicit

Z3Wire never implicitly converts between types. There are no implicit
conversions between:

- `Bool` and `Ubv<1>`
- Signed and unsigned bit-vectors
- Bit-vectors of different widths

All conversions must go through the casting API. The one exception is
concrete-to-symbolic promotion in mixed expressions (see above).
