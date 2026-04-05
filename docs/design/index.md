# Design

Design decision records for Z3Wire. Each document captures a tricky design
space, the reasoning behind the decisions made, and alternatives considered.

- [Lossless Auto-Promotion](lossless-auto-promotion.md) — which type conversions
    happen implicitly, which require explicit casts, and why
- [Mathematical Comparison](mathematical-comparison.md) — why comparisons
    operate on mathematical values rather than requiring matching types
- [Bit-Growth Arithmetic](bit-growth-arithmetic.md) — why arithmetic and left
    shift widen their result types instead of wrapping
