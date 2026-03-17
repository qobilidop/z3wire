# Types

Z3Wire provides symbolic types that wrap Z3 expressions, concrete types that
store plain values, and a three-tier casting API for converting between them.

## Symbolic types

Z3Wire symbolic types are simple wrappers around `z3::expr`.

| Type              | Description                               |
| :---------------- | :---------------------------------------- |
| `z3w::SymBool`    | Symbolic boolean                          |
| `z3w::SymUInt<W>` | Symbolic unsigned bit-vector of width `W` |
| `z3w::SymSInt<W>` | Symbolic signed bit-vector of width `W`   |

!!! note

    `SymUInt<0>` and `SymSInt<0>` are forbidden at compile time. A zero-width bit-vector has no meaning in SMT and is not supported in Z3.

### Constructors

Symbolic variables of any type can be constructed with a Z3 context and a
symbol name:

```cpp
z3::context ctx;

z3w::SymBool flag(ctx, "flag");
z3w::SymUInt<10> address(ctx, "address");
z3w::SymSInt<11> offset(ctx, "offset");
```

### Literals

`SymBool` literals:

```cpp
auto t = z3w::SymBool::True(ctx);
auto f = z3w::SymBool::False(ctx);
```

`SymUInt`/`SymSInt` literals are range-checked at compile time:

```cpp
auto a = z3w::SymUInt<8>::Literal<255>(ctx);  // OK
auto b = z3w::SymUInt<8>::Literal<256>(ctx);  // Compile error: value doesn't fit
auto c = z3w::SymSInt<8>::Literal<127>(ctx);  // OK
auto d = z3w::SymSInt<8>::Literal<128>(ctx);  // Compile error: value doesn't fit
```

### Z3 interop

Use `.raw()` to get the underlying `z3::expr` for interop with Z3 APIs:

```cpp
z3w::SymUInt<8> x(ctx, "x");
solver.add(x.raw() > 0);  // Pass to Z3 solver directly
```

!!! note

    Z3Wire does not wrap `z3::context` or `z3::solver`. You interact with them
    directly.

## Concrete types

Fixed-width integer value holders. Useful for storing typed values and mixing
concrete values into symbolic expressions.

| Type           | Description                                    |
| :------------- | :--------------------------------------------- |
| `z3w::Bool`    | Type-safe boolean wrapper                      |
| `z3w::UInt<W>` | Unsigned concrete integer of width `W` (1--64) |
| `z3w::SInt<W>` | Signed concrete integer of width `W` (1--64)   |

`UInt<W>` and `SInt<W>` are aliases for the underlying template:

```cpp
template <size_t W, bool IsSigned>
class BitVec;

template <size_t W> using UInt = BitVec<W, false>;
template <size_t W> using SInt = BitVec<W, true>;
```

!!! info

    Unlike symbolic types, concrete types do not require a `z3::context`.
    They store plain native values, not Z3 expressions.

### Concrete `Bool`

`z3w::Bool` is a type-safe wrapper around native `bool` that prevents implicit
construction from non-boolean types.

```cpp
z3w::Bool b = true;         // OK: implicit from bool
z3w::Bool b2 = false;       // OK
z3w::Bool b3;               // OK: default-initializes to false
z3w::Bool b4 = 42;          // Compile error: deleted integral constructor
```

Accessing the value:

```cpp
z3w::Bool b = true;
b.value();                   // true
if (b) { /* ... */ }         // OK: contextual conversion to bool
bool native = bool(b);       // OK: explicit operator bool()
```

Equality with `bool` is supported for testing convenience:

```cpp
z3w::Bool b = true;
b == true;   // true
b != false;  // true
```

Stream output prints `"true"` or `"false"`.

### Integer construction

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
// val.bits() == 44, truncated == true

auto [val2, truncated2] = z3w::UInt<8>::checked(200);
// val2.bits() == 200, truncated2 == false

auto [val3, truncated3] = z3w::SInt<8>::checked(-128);
// val3.value() == -128, truncated3 == false
```

`checked()` accepts any integer type and reports whether the value was
truncated to fit. Negative values on unsigned types are flagged as truncated.

### Accessing the value

Two accessors:

- `.bits()` — raw unsigned bit pattern (always `UnsignedStorageType<W>`)
- `.value()` — interpreted value (unsigned for `UInt`, signed for `SInt`)

```cpp
auto x = z3w::UInt<8>::Literal<200>();
x.bits();   // 200 (uint8_t)
x.value();  // 200 (uint8_t) — same as bits() for unsigned

auto y = z3w::SInt<8>::Literal<-1>();
y.bits();   // 0xFF (uint8_t) — raw two's complement
y.value();  // -1   (int8_t)  — interpreted signed value
```

### Mixing with symbolic types

Concrete values automatically promote to symbolic when used in expressions
with symbolic operands:

```cpp
z3::context ctx;
z3w::SymUInt<8> sym(ctx, "x");
auto conc = z3w::UInt<8>::Literal<42>();

auto result = sym + conc;  // SymUInt<9> — concrete promoted automatically
z3w::SymBool eq = (sym == conc);  // Symbolic equality check
```

The promotion grabs the `z3::context` from the symbolic operand, so no
context needs to be passed explicitly.

!!! note

    This is the one exception to the "explicit over implicit" rule: concrete-
    to-symbolic promotion is always lossless and unambiguous (same width,
    same signedness), so it is allowed implicitly.

To convert a concrete value to symbolic manually:

```cpp
auto conc = z3w::UInt<8>::Literal<42>();
z3w::SymUInt<8> sym = z3w::to_symbolic(conc, ctx);
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

Z3Wire provides a three-tier casting API for symbolic types, giving you explicit
control over how bit-vectors change width or signedness.

### `cast<T>(val)` — Hardware cast

Raw truncation, extension, or sign reinterpretation. No safety checks. Use when
you intentionally want hardware-style overflow or wrap behavior.

```cpp
z3w::SymUInt<16> wide(ctx, "wide");
auto narrow = z3w::cast<z3w::SymUInt<8>>(wide);   // Truncate to 8 bits
auto wider = z3w::cast<z3w::SymUInt<32>>(wide);    // Zero-extend to 32 bits
auto reint = z3w::cast<z3w::SymSInt<16>>(wide);    // Reinterpret as signed
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
z3w::SymUInt<8> small(ctx, "small");
auto wide = z3w::safe_cast<z3w::SymUInt<16>>(small);   // OK: widening
auto bad = z3w::safe_cast<z3w::SymUInt<4>>(small);      // Compile error!
```

**Rules:**

| Source        | Target        | Allowed?                                       |
| :------------ | :------------ | :--------------------------------------------- |
| `SymUInt<W1>` | `SymUInt<W2>` | Yes, if `W2 >= W1`                             |
| `SymSInt<W1>` | `SymSInt<W2>` | Yes, if `W2 >= W1`                             |
| `SymUInt<W1>` | `SymSInt<W2>` | Yes, if `W2 > W1` (needs 1 extra bit for sign) |
| `SymSInt<W1>` | `SymUInt<W2>` | **Always forbidden** (negative values corrupt) |
| Any           | Smaller       | **Always forbidden** (truncation is not safe)  |

### `checked_cast<T>(val)` — Verification cast

Performs the cast and returns a symbolic boolean indicating whether data loss
occurred. Use when you want to verify safety as part of a Z3 proof.

```cpp
z3w::SymUInt<16> val(ctx, "val");
auto [result, overflowed] = z3w::checked_cast<z3w::SymUInt<8>>(val);

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

## SymBool / bit-vector conversion

In Z3, `SymBool` and a 1-bit bit-vector are distinct sorts. Hardware frequently
needs to convert between them. Z3Wire provides explicit conversion functions:

```cpp
// Bit-vector to SymBool
z3w::SymUInt<32> status(ctx, "status");
z3w::SymBool ready = z3w::to_bool(z3w::extract<0, 0>(status));

// SymBool to bit-vector
z3w::SymBool cond(ctx, "cond");
z3w::SymUInt<1> flag = z3w::to_ubv1(cond);
```

## Design principle: explicit over implicit

Z3Wire never implicitly converts between types. There are no implicit
conversions between:

- `SymBool` and `SymUInt<1>`
- Signed and unsigned bit-vectors
- Bit-vectors of different widths

All conversions must go through the casting API. The one exception is
concrete-to-symbolic promotion in mixed expressions (see above).
