# Shifting

Z3Wire provides a three-tier shift API, mirroring the
[casting tiers](casting.md).

## `<<`, `>>` — Hardware shift

Raw hardware shift. Width stays constant, bits that shift out are silently lost.
Operands must have the same width and signedness.

- **Left shift (`<<`):** Logical shift for both `Ubv` and `Sbv`.
- **Right shift (`>>`):** Logical shift (`lshr`) for `Ubv`, arithmetic shift
  (`ashr`) for `Sbv` (preserves the sign bit).

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<8> n(ctx, "n");
auto result = a << n;  // Ubv<8>, bits may be lost
auto shifted = a >> n;  // Ubv<8>, zero-filled from the left
```

## `checked_shl`, `checked_shr` — Checked shift

Performs the shift and returns a symbolic `Bool` indicating whether any non-zero
bits were lost. Width stays constant.

```cpp
z3w::Ubv<8> val(ctx, "val");
z3w::Ubv<8> amount(ctx, "amount");

auto [shifted, lost] = z3w::checked_shl(val, amount);
// shifted: Ubv<8>
// lost: Bool (true if any bits were shifted out)

solver.add(!lost.raw());  // Assert: this shift never loses bits
```

## `lossless_shl` — Lossless left shift

The result type is wide enough to guarantee no bits are ever lost.

### Constant shift

Result width = `W + N`:

```cpp
z3w::Ubv<8> a(ctx, "a");
auto r = z3w::lossless_shl<3>(a);  // Ubv<11>
```

### Symbolic shift

Result width = `W + 2^K - 1`, where `K` is the shift amount width. This
accounts for the maximum possible shift.

```cpp
z3w::Ubv<8> a(ctx, "a");
z3w::Ubv<3> n(ctx, "n");      // Can shift by 0..7
auto r = z3w::lossless_shl(a, n);  // Ubv<15> (8 + 2^3 - 1)
```

## Choosing the right tier

| Situation | Use |
|:----------|:----|
| Modeling hardware shift registers (intentional truncation) | `<<`, `>>` |
| Need to prove no bits are lost symbolically | `checked_shl`, `checked_shr` |
| Want the compiler to guarantee no loss | `lossless_shl` |
