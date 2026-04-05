# Bit-Growth Arithmetic

Why Z3Wire arithmetic and left shift widen their result types instead of
wrapping at fixed width.

## The decision

Arithmetic operators (`+`, `-`, unary `-`, `*`) and left shift (`shl`) produce
result types wider than their inputs. The result width is the minimum that
guarantees no overflow can occur — the true mathematical result always fits.

For the complete typing rules, see the
[Operations](../usage/operations.md#arithmetic) usage page.

## Why

### Overflow is invisible in fixed-width arithmetic

In hardware and most programming languages, arithmetic wraps at a fixed width.
The result of `200 + 100` in 8-bit unsigned is `44`, with no indication that
overflow occurred. Detecting overflow requires separate logic — carry flags,
wider intermediate values, or manual range checks.

Z3Wire is a verification library. Its job is to make properties easy to state
and bugs hard to miss. If arithmetic silently wraps, the programmer must
remember to check for overflow everywhere — exactly the class of bug that
verification is supposed to catch.

### Bit-growth makes overflow impossible by construction

By widening the result, Z3Wire guarantees that the mathematical result is always
representable. Overflow doesn't happen because the type is wide enough to hold
every possible output.

This shifts the question from "did overflow occur?" (a runtime property the
programmer must remember to check) to "does this value fit in my target width?"
(an explicit truncation that `checked_cast` makes visible).

### The truncation is explicit

When the programmer wants a fixed-width result (e.g., feeding back into an 8-bit
register), they truncate explicitly:

```cpp
auto sum = a + b;                                          // SymUInt<9>
auto [truncated, ok] = z3w::checked_cast<z3w::SymUInt<8>>(sum);  // explicit
```

`checked_cast` returns a flag indicating whether the truncation was lossless.
This is the verification-friendly alternative to a carry flag — the information
is in the type system, not a side channel.

## Which operations use bit-growth

| Operation   | Result width  | Rationale                                        |
| ----------- | ------------- | ------------------------------------------------ |
| `a + b`     | max(A, B) + 1 | Sum of two W-bit values needs at most W+1 bits   |
| `a - b`     | max(A, B) + 1 | Difference may be negative, needs sign bit       |
| `-a`        | A + 1         | Negation of min signed value needs one more bit  |
| `a * b`     | A + B         | Product of A-bit and B-bit values needs A+B bits |
| `shl<N>(a)` | W + N         | Left shift by N is multiplication by 2^N         |
| `shl(a, n)` | W + 2^K − 1   | Maximum shift amount is 2^K − 1                  |

These are the minimum widths that guarantee lossless results for every possible
input. No wider than necessary — the type encodes the tightest bound.

## Contrast with other operators

| Operator class | Width policy     | Why                                                          |
| -------------- | ---------------- | ------------------------------------------------------------ |
| Arithmetic     | Bit-growth       | Result width encodes overflow capacity                       |
| Left shift     | Bit-growth       | Shifting is multiplication by a power of two                 |
| Right shift    | Width-preserving | Shifting right discards low bits; the result fits in W bits  |
| Rotation       | Width-preserving | Bit rotation is a permutation; no information gained or lost |
| Bitwise        | Strict match     | Bit positions must align; widening changes semantics         |
| Comparison     | Auto-widen       | Asks about values; result is always `SymBool`                |

Right shift and rotation are width-preserving because no new information is
created — the result is always a subset of the input's value set.

## Signedness rules

The result signedness follows from value set analysis — the result type must be
the narrowest type whose value set contains all possible outputs. The detailed
typing rules (including the mixed signed/unsigned cases) follow the CIRCT
`hwarith` dialect and are documented on the
[Operations](../usage/operations.md#arithmetic) usage page.

The key insight: unsigned minus unsigned produces a signed result, because the
difference can be negative. This is one of the cases where the type system
prevents a subtle bug — hardware subtraction of unsigned values wraps silently,
but Z3Wire's result type makes the sign explicit.

## Connection to verification patterns

Bit-growth arithmetic enables the core verification pattern in Z3Wire:

1. **Compute the true result** — bit-growth guarantees it's exact.
1. **Truncate to hardware width** — `checked_cast` makes this explicit.
1. **Compare hardware vs specification** — mathematical comparison handles the
    width mismatch.

This is exactly the pattern in the adder example (`examples/adder.cc`): the
specification uses `a + b` (9-bit, exact) and `checked_cast` (explicit
truncation), while the hardware implementation manually manages the carry. Z3
proves they agree.

See also:

- [Mathematical Comparison](mathematical-comparison.md) — how comparisons handle
    the width differences that bit-growth creates
- [Lossless Auto-Promotion](lossless-auto-promotion.md) — the broader
    auto-promotion strategy
