# Bit-Field Equality

## Motivation

Hardware registers, protocol headers, and wire formats are commonly specified as
a flat bit-vector subdivided into named fields. Z3Wire already provides `extract`
and `concat` for manual field access, but establishing the bidirectional
relationship between a buffer and its fields requires boilerplate:

```cpp
z3w::Ubv<8> md(ctx, "md");
z3w::Bool x(ctx, "x");
z3w::Ubv<3> y(ctx, "y");
z3w::Sbv<4> z(ctx, "z");

// Manual: verbose and error-prone
solver.add((md == z3w::concat(
    z3w::cast<z3w::Ubv<4>>(z),
    y,
    z3w::to_ubv1(x)
)).raw());
```

`bitfield_eq` eliminates this boilerplate with a single, type-safe call.

## API

```cpp
template <size_t W, typename... Fields>
z3w::Bool bitfield_eq(const z3w::Ubv<W>& buffer, const Fields&... fields);
```

### Usage

```cpp
z3w::Ubv<8> md(ctx, "md");
z3w::Bool x(ctx, "x");       // bit 0
z3w::Ubv<3> y(ctx, "y");     // bits 3..1
z3w::Sbv<4> z(ctx, "z");     // bits 7..4

solver.add(z3w::bitfield_eq(md, x, y, z).raw());
```

Once asserted, constraining any field propagates to the buffer and vice versa
-- Z3 handles the bidirectional reasoning automatically.

## Design decisions

### Returns a constraint, does not take a solver

`bitfield_eq` returns a `z3w::Bool` rather than taking a `z3::solver` and
asserting directly. This is consistent with the rest of Z3Wire, which never
wraps `z3::solver`. Returning a constraint also gives the user full
composability -- they can negate it, combine it with other expressions, or
choose when to assert it.

### LSB-first field ordering

Fields are mapped least-significant-bit first: the first field argument
occupies the lowest bits, the last field occupies the highest bits.

This matches the dominant modern convention used by Amaranth (`Struct`),
Chisel (`Bundle`), and FIRRTL (`BundleType`). SystemVerilog's `packed struct`
uses MSB-first, but that is the legacy outlier.

Note that this is the **opposite** of `concat`, which places its first
argument in the most significant position (matching Z3 and SMT-LIB
convention). This mismatch is intentional and consistent with how other tools
handle the two operations. Documentation should make this explicit.

### Automatic type conversions

`bitfield_eq` accepts `Bool`, `Ubv<N>`, and `Sbv<N>` fields and handles
conversions internally:

- `Bool` fields are converted to `Ubv<1>` via `to_ubv1`.
- `Sbv<N>` fields are reinterpreted as `Ubv<N>` via `cast` (same bits,
    different type).
- `Ubv<N>` fields are used as-is.

This keeps the call site clean -- users don't need to manually convert field
types before calling `bitfield_eq`.

### Compile-time width check

A `static_assert` verifies that the field widths sum to the buffer width. A
mismatch is a compile error, not a runtime surprise.

### Symbolic types only

`bitfield_eq` operates on symbolic types (`Bool`, `Ubv`, `Sbv`). Concrete
types (`UInt`, `SInt`) are out of scope -- bit-field decomposition is a
constraint-based operation that only makes sense in a symbolic context.

### No solver interaction

Z3Wire does not wrap `z3::context` or `z3::solver`. `bitfield_eq` follows
this principle: it constructs a Z3 expression from its arguments and returns
it. The user decides what to do with the constraint.

### No nesting

`bitfield_eq` is flat -- all fields must be individual `Bool`, `Ubv`, or `Sbv`
values. Grouping is already available via `concat`, and multi-level
decomposition is achieved by chaining `bitfield_eq` calls:

```cpp
z3w::Ubv<16> full(ctx, "full");
z3w::Ubv<8> lo(ctx, "lo");
z3w::Ubv<8> hi(ctx, "hi");
z3w::Bool x(ctx, "x");
z3w::Ubv<3> y(ctx, "y");
z3w::Sbv<4> z(ctx, "z");

solver.add(z3w::bitfield_eq(full, lo, hi).raw());
solver.add(z3w::bitfield_eq(lo, x, y, z).raw());
```

### No MSB-first variant

YAGNI. LSB-first covers the common case. An MSB-first variant can be added
later if demand arises.

## Alternatives considered

### Macro-generated struct

A macro like `Z3W_BITFIELD(Metadata, (Bool, x), (Ubv<3>, y), (Sbv<4>, z))`
could generate a struct with named fields and a backing bit-vector. This would
give `md.x` access syntax, but introduces macro complexity and a non-standard
type definition pattern. Rejected in favor of the simpler function approach.

### Template struct with index access

A `z3w::BitField<Bool, Ubv<3>, Sbv<4>>` template with `get<0>()` access.
This avoids macros but loses named fields, making code harder to read.
Rejected for ergonomic reasons.

### Plain C++ struct with reflection

The ideal API: define a plain `struct` with Z3Wire members and let the library
discover the fields automatically. This requires compile-time reflection,
which arrives in C++26 (P2996). Not feasible in C++20.

As a workaround, users can define a plain aggregate struct and use C++
structured bindings to destructure it before passing to `bitfield_eq`:

```cpp
struct MdFields {
    z3w::Bool x;
    z3w::Ubv<3> y;
    z3w::Sbv<4> z;
};

MdFields md_struct{
    z3w::Bool(ctx, "x"),
    z3w::Ubv<3>(ctx, "y"),
    z3w::Sbv<4>(ctx, "z"),
};

auto& [x, y, z] = md_struct;
solver.add(z3w::bitfield_eq(md_buffer, x, y, z).raw());
```

## Future directions

- **MSB-first variant** if use cases arise.
- **C++26 reflection** could enable `bitfield_eq(buffer, my_struct)` with a
    plain aggregate struct, eliminating the need for manual destructuring.
