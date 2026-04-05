# Design

Design decision records for Z3Wire. Each document captures a tricky design
space, the reasoning behind the decisions made, and alternatives considered.

- [Concrete Types](concrete-types.md) — why concrete types exist, why they have
    minimal operations, and their role as typed data holders
- [Bool vs `UInt<1>`](bool-vs-uint1.md) — why booleans and 1-bit vectors are
    separate types with explicit conversion
- [Lossless Auto-Promotion](lossless-auto-promotion.md) — which type conversions
    happen implicitly, which require explicit casts, and why
- [Three-Tier Casting](three-tier-casting.md) — why there are three cast
    functions and when to use each
- [Mathematical Comparison](mathematical-comparison.md) — why comparisons
    operate on mathematical values rather than requiring matching types
- [Bit-Growth Arithmetic](bit-growth-arithmetic.md) — why arithmetic and left
    shift widen their result types instead of wrapping
