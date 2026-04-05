# Mathematical Comparison

Why Z3Wire comparisons operate on mathematical values rather than requiring
matching types.

## The decision

All six comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`) accept operands
of any width and signedness combination. Both operands are widened to a common
type before comparing, so the comparison always answers the question: **do these
operands represent the same mathematical value (or stand in the correct
ordering)?**

```cpp
auto sum = a + b;                            // SymUInt<9>
auto [truncated, ok] = checked_cast<SymUInt<8>>(sum);
SymBool matches = (sum == truncated);        // SymUInt<9> vs SymUInt<8>: works
SymBool overflow = (sum > UInt<8>::Literal<255>());  // SymUInt<9> vs UInt<8>: works
```

No manual casting is needed. The programmer states the mathematical question;
the type system handles the representation.

## Why

In verification, the natural questions are about values:

- "Does this 9-bit sum equal this 8-bit input?" (checking for overflow)
- "Is this unsigned counter less than this signed threshold?"
- "Does the hardware output match the specification?"

Requiring the programmer to manually match widths before comparing would add
noise without adding safety. Unlike arithmetic (where the result width matters)
or bitwise operations (where bit positions must align), comparisons produce a
`SymBool` — there is no result type to be surprised by.

## The widening rule

Both operands are extended to a **common width** that preserves every value from
both sides:

| Operand signedness | Common width    | Extension method                             |
| ------------------ | --------------- | -------------------------------------------- |
| Both unsigned      | max(W1, W2)     | Narrower side zero-extended                  |
| Both signed        | max(W1, W2)     | Narrower side sign-extended                  |
| Mixed              | max(W1, W2) + 1 | Unsigned zero-extended, signed sign-extended |

The mixed-signedness case needs one extra bit because the unsigned operand's
full range (0 to 2^W − 1) requires W + 1 signed bits to represent without loss.

### Ordered comparison semantics

For ordered comparisons (`<`, `<=`, `>`, `>=`), the signedness of the comparison
itself is determined by the operands:

- **Both unsigned** — unsigned comparison (`ult`, `ule`, `ugt`, `uge`)
- **Either signed** — signed comparison (`slt`, `sle`, `sgt`, `sge`)

After widening to the common width, the operands have matching signedness (the
extra bit in the mixed case ensures the unsigned values are correctly
represented in the signed domain). The comparison then uses the appropriate Z3
operation.

## Contrast with other operators

| Operator class | Width policy | Why                                                  |
| -------------- | ------------ | ---------------------------------------------------- |
| Comparison     | Auto-widen   | Asks about values; no result type to surprise        |
| Arithmetic     | Bit-growth   | Result width encodes overflow capacity               |
| Bitwise        | Strict match | Bit positions must align; widening changes semantics |

Bitwise operations require strict width matching because zero-extending an
operand changes which bits participate. `0xFF & x` means something different for
8-bit x vs 16-bit x. Comparisons have no such ambiguity — the mathematical value
is independent of the representation width.

## Connection to auto-promotion

Mathematical comparison is what makes several ergonomic patterns work:

- **Concrete literals in comparisons** —
    `ethertype == UInt<16>::Literal<0x0800>()` works because the concrete side
    is promoted to symbolic (same width), and the comparison auto-widens if
    needed.

- **Checked cast verification** — `sum == truncated` where `sum` is wider than
    `truncated` is the natural way to ask "did truncation lose information?"

- **Future: native integer comparisons** — `ethertype == 0x0800` would convert
    the native int to a 32-bit concrete value, promote to symbolic, and
    auto-widen for comparison. The mathematical semantics make this chain
    lossless and correct.

See [Lossless Auto-Promotion](lossless-auto-promotion.md) for the full
auto-promotion strategy.
