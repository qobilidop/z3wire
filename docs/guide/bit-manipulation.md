# Bit Manipulation

## Extract (Bit Slicing)

### Static Extract

Extract a contiguous range of bits with compile-time bounds:

```cpp
template <size_t High, size_t Low, size_t W, bool S>
Ubv<High - Low + 1> extract(const BitVec<W, S>& val);
```

Bounds are checked at compile time: `High >= Low` and `High < W`.

```cpp
z3w::Ubv<32> instruction(ctx, "instruction");

auto opcode = z3w::extract<31, 24>(instruction);  // Ubv<8>
auto reg = z3w::extract<3, 0>(instruction);        // Ubv<4>
auto bit5 = z3w::extract<5, 5>(instruction);       // Ubv<1>
```

!!! note
    The result is always `Ubv` (unsigned). Extracted bits have no inherent
    signedness.

### Symbolic-Offset Extract

Extract a fixed number of bits starting at a symbolic position:

```cpp
template <size_t TargetWidth, size_t W, bool S, size_t IdxW>
Ubv<TargetWidth> extract(const BitVec<W, S>& val, const Ubv<IdxW>& start_idx);
```

Implemented via barrel-shifting: shift right by the symbolic offset, then
statically extract the bottom bits.

```cpp
z3w::Ubv<32> data(ctx, "data");
z3w::Ubv<5> offset(ctx, "offset");

auto nibble = z3w::extract<4>(data, offset);  // Ubv<4>
auto byte = z3w::extract<8>(data, offset);    // Ubv<8>
```

## Concatenation

Glue bit-vectors together. The result width is `W1 + W2`, always returned as
`Ubv`.

```cpp
z3w::Ubv<16> high(ctx, "high");
z3w::Ubv<16> low(ctx, "low");
auto full = z3w::concat(high, low);  // Ubv<32>
```

Supports variadic arguments:

```cpp
z3w::Ubv<4> a(ctx, "a");
z3w::Ubv<4> b(ctx, "b");
z3w::Ubv<8> c(ctx, "c");
auto packed = z3w::concat(a, b, c);  // Ubv<16>
```

!!! tip
    `concat` and `extract` are complementary. A common hardware pattern is
    unpacking a word into fields, modifying a field, and repacking:

    ```cpp
    z3w::Ubv<16> word(ctx, "word");
    auto hi = z3w::extract<15, 8>(word);  // Ubv<8>
    auto lo = z3w::extract<7, 0>(word);   // Ubv<8>
    auto repacked = z3w::concat(hi, lo);  // Ubv<16>
    ```
