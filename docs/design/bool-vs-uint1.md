# Bool vs `UInt<1>`

Why `Bool`/`SymBool` and `UInt<1>`/`SymUInt<1>` are separate types with explicit
conversion, despite having the same number of values.

## The decision

`SymBool` and `SymUInt<1>` are distinct types. Converting between them requires
explicit function calls: `as_bool()` and `as_uint1()`. There is no implicit
conversion in either direction.

## Why they're separate

### Different Z3 sorts

In Z3, booleans and 1-bit bit-vectors are distinct sorts. A Z3 boolean
expression cannot be directly used where a bit-vector is expected, and vice
versa. Z3Wire's type separation reflects this underlying reality — collapsing
them into one type would require hidden conversions at the Z3 API boundary.

### Different semantic domains

`SymBool` represents a logical proposition — the result of a comparison, a
satisfiability condition, or a logical connective. Its operators are logical:
`&&`, `||`, `!`.

`SymUInt<1>` represents a 1-bit hardware wire — a single bit in a bit-vector.
Its operators are bitwise: `&`, `|`, `~`.

Although both have two values, they answer different questions. A `SymBool` asks
"is this true?" A `SymUInt<1>` asks "is this bit set?"

### Different operator sets

| Operation  | `SymBool`            | `SymUInt<1>`               |
| ---------- | -------------------- | -------------------------- |
| AND        | `a && b` (logical)   | `a & b` (bitwise)          |
| OR         | `a \|\| b` (logical) | `a \| b` (bitwise)         |
| NOT        | `!a` (logical)       | `~a` (bitwise)             |
| XOR        | `a ^ b`              | `a ^ b`                    |
| Comparison | `a == b`, `a != b`   | All six (`==`, `<`, ...)   |
| Arithmetic | None                 | `+`, `-`, `*` (bit-growth) |
| Concat     | As 1-bit input       | As 1-bit input             |

Merging the types would require either overloading `&&` and `&` on the same type
(confusing) or dropping one operator set (limiting).

## Why conversion is explicit

The value sets biject ({false, true} ↔ {0, 1}), so the conversion is always
lossless. The [lossless auto-promotion](lossless-auto-promotion.md) principle
says lossless conversions should happen automatically. Why the exception?

**Semantic clarity outweighs brevity.** An implicit conversion would hide the
boundary between "logical reasoning" and "hardware bit manipulation." When a
programmer writes `as_uint1(carry_flag)`, they're declaring intent: "I'm moving
from the logical domain to the bit domain." This makes code self-documenting at
the boundary — which matters especially in verification, where mixing up domains
can produce correct-looking but meaningless formulas.

**The friction is small and localized.** The conversion typically happens at
specific boundary points — extracting a bit to use as a boolean condition, or
converting a comparison result to feed into a bit-vector operation. These are
one-off calls, not pervasive noise.

## Real usage patterns

The adder example (`examples/adder.cc`) illustrates both directions:

- Gate-level logic uses `SymUInt<1>` throughout — bits flow through `&`, `|`,
    `^` gates.
- The final equivalence check uses `SymBool` — comparisons (`!=`, `||`) produce
    logical propositions for the solver.
- The boundary: `as_uint1(!value_preserved)` converts the comparison result
    (logical) into a bit (hardware) to compare against the carry output.

The barrel shifter example (`examples/barrel_shifter.cc`) shows the same
pattern: `SymUInt<1>` for mux select lines and data bits, `SymBool` for the
solver query.

## Connection to other design decisions

The bool/uint1 distinction aligns with the overall type system philosophy:

- **Explicit over implicit** — conversions between semantically distinct types
    require explicit intent, even when lossless.
- **Operator sets track semantics** — logical operators on `SymBool`, bitwise on
    `SymUInt<1>`, arithmetic on wider bit-vectors. Each type's operators match
    its domain.
- **Z3 fidelity** — Z3Wire's types mirror Z3's sort system rather than
    abstracting it away. This keeps the mapping transparent and avoids hidden
    translation overhead.
