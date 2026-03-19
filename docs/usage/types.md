# Types

Z3Wire provides symbolic types that wrap Z3 expressions and concrete types that
store native values.

## Overview

| Symbolic (wraps `z3::expr`) | Concrete (stores native value) |
| :-------------------------- | :----------------------------- |
| `z3w::SymBool`              | `z3w::Bool`                    |
| `z3w::SymUInt<W>`           | `z3w::UInt<W>`                 |
| `z3w::SymSInt<W>`           | `z3w::SInt<W>`                 |

## Symbolic types

Symbolic types are thin wrappers around `z3::expr`. Bit-vector width must be at
least 1 (`SymUInt<0>` and `SymSInt<0>` are forbidden at compile time).

### Construction

Symbolic variables are constructed with a Z3 context and a symbol name:

```cpp
z3::context ctx;

z3w::SymBool flag(ctx, "flag");
z3w::SymUInt<8> x(ctx, "x");
z3w::SymSInt<8> y(ctx, "y");
```

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

Use `.expr()` to get the underlying `z3::expr` for interop with Z3 APIs:

```cpp
z3::solver solver(ctx);
z3w::SymUInt<8> x(ctx, "x");
solver.add(x.expr() > 0);  // Pass to Z3 solver directly
```

To wrap a raw `z3::expr` back into a Z3Wire type, use `FromExpr`:

```cpp
z3::expr raw = /* ... from Z3 API ... */;

// Returns std::optional — nullopt if sort or width doesn't match.
std::optional<z3w::SymUInt<8>> x = z3w::SymUInt<8>::FromExpr(raw);
std::optional<z3w::SymBool> b = z3w::SymBool::FromExpr(raw);
```

!!! note

    Z3Wire does not wrap `z3::context` or `z3::solver`. You interact with them
    directly.

## Concrete types

Concrete types store native values and do not require a `z3::context`.
Bit-vector width must be between 1 and 64.

### Construction

`Bool` is constructed from `bool` (non-boolean types are rejected at compile
time):

```cpp
z3w::Bool flag = true;   // OK: implicit from bool
z3w::Bool flag2 = 42;    // Compile error: only bool accepted
```

`UInt`/`SInt` literals are range-checked at compile time:

```cpp
auto a = z3w::UInt<8>::Literal<255>();   // OK
auto b = z3w::UInt<8>::Literal<256>();   // Compile error: value doesn't fit
auto c = z3w::SInt<8>::Literal<127>();   // OK
auto d = z3w::SInt<8>::Literal<128>();   // Compile error: value doesn't fit
```

Runtime checked:

```cpp
auto [x, x_truncated] = z3w::UInt<8>::Checked(300);
// x.value() == 44, x_truncated == true
auto [y, y_truncated] = z3w::UInt<8>::Checked(200);
// y.value() == 200, y_truncated == false
```

### Value access

`.value()` returns the stored value in its natural type:

```cpp
z3w::Bool flag = true;
flag.value();  // true

auto x = z3w::UInt<8>::Literal<200>();
x.value();  // 200 (uint8_t)

auto y = z3w::SInt<8>::Literal<-1>();
y.value();  // -1 (int8_t)
```

`Bool` also supports contextual conversion to `bool`:

```cpp
z3w::Bool flag = true;
if (flag) { /* ... */ }  // OK: contextual conversion to bool
```
