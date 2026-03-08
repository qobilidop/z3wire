# Concrete bit-vector types implementation plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add concrete fixed-width integer types (`UInt<W>`, `SInt<W>`) that mirror the symbolic `Ubv<W>`/`Sbv<W>` API and support implicit promotion to symbolic in mixed expressions.

**Architecture:** Standalone `Int<W, IsSigned>` template in `z3wire/int.h` storing a masked unsigned native integer. Mixed concrete+symbolic overloads added to `z3wire/bitvec.h` using C++20 concepts. See `docs/plans/2026-03-08-concrete-types-design.md` for rationale.

**Tech Stack:** C++20 (clang), Bazel + CMake, Google Test, Z3

---

### Task 1: Scaffold `Int` class with storage and construction

**Files:**
- Create: `z3wire/int.h`
- Create: `z3wire/int_test.cc`
- Modify: `z3wire/BUILD.bazel`
- Modify: `z3wire/CMakeLists.txt`

**Step 1: Write failing tests for construction**

Create `z3wire/int_test.cc`:

```cpp
#include "z3wire/int.h"

#include <gtest/gtest.h>

namespace z3w {
namespace {

// --- Storage type ---

TEST(IntTest, StorageType) {
  static_assert(std::is_same_v<StorageType<1>, uint8_t>);
  static_assert(std::is_same_v<StorageType<8>, uint8_t>);
  static_assert(std::is_same_v<StorageType<9>, uint16_t>);
  static_assert(std::is_same_v<StorageType<16>, uint16_t>);
  static_assert(std::is_same_v<StorageType<17>, uint32_t>);
  static_assert(std::is_same_v<StorageType<32>, uint32_t>);
  static_assert(std::is_same_v<StorageType<33>, uint64_t>);
  static_assert(std::is_same_v<StorageType<64>, uint64_t>);
}

// --- Type traits ---

TEST(IntTest, TypeTraits) {
  static_assert(UInt<8>::kWidth == 8);
  static_assert(!UInt<8>::kIsSigned);
  static_assert(SInt<8>::kWidth == 8);
  static_assert(SInt<8>::kIsSigned);
}

// --- Raw constructor (masking) ---

TEST(IntTest, RawConstructorMasks) {
  UInt<8> a(300);
  EXPECT_EQ(a.value(), 44);  // 300 & 0xFF
}

TEST(IntTest, RawConstructorNonPowerOfTwo) {
  UInt<5> a(0xFF);
  EXPECT_EQ(a.value(), 0x1F);  // 0xFF & 0x1F
}

TEST(IntTest, RawConstructorSigned) {
  SInt<8> a(300);
  EXPECT_EQ(a.value(), 44);  // Same underlying bits
}

// --- Literal (compile-time checked) ---

TEST(IntTest, LiteralUnsigned) {
  auto a = UInt<8>::Literal<255>();
  EXPECT_EQ(a.value(), 255);
}

TEST(IntTest, LiteralZero) {
  auto a = UInt<8>::Literal<0>();
  EXPECT_EQ(a.value(), 0);
}

TEST(IntTest, LiteralSigned) {
  // SInt<8> range: -128 to 127.
  auto a = SInt<8>::Literal<127>();
  EXPECT_EQ(a.value(), 127);
}

// --- Checked construction ---

TEST(IntTest, CheckedNoTruncation) {
  auto [val, truncated] = UInt<8>::checked(200);
  EXPECT_EQ(val.value(), 200);
  EXPECT_FALSE(truncated);
}

TEST(IntTest, CheckedWithTruncation) {
  auto [val, truncated] = UInt<8>::checked(300);
  EXPECT_EQ(val.value(), 44);
  EXPECT_TRUE(truncated);
}

}  // namespace
}  // namespace z3w
```

**Step 2: Add build targets**

Add to `z3wire/BUILD.bazel` (after existing `bitvec_test`):

```python
cc_test(
    name = "int_test",
    srcs = ["int_test.cc"],
    deps = [
        ":z3wire",
        "@googletest//:gtest_main",
    ],
)
```

Add to `z3wire/CMakeLists.txt` (after `bitvec_test`):

```cmake
    add_executable(int_test int_test.cc)
    target_link_libraries(int_test PRIVATE z3wire GTest::gtest_main)
    add_test(NAME int_test COMMAND int_test)
```

**Step 3: Run tests to verify they fail**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: FAIL (file not found / compilation errors)

**Step 4: Implement `Int` class with construction**

Create `z3wire/int.h`:

```cpp
#ifndef Z3WIRE_INT_H_
#define Z3WIRE_INT_H_

#include <cstdint>
#include <type_traits>
#include <utility>

namespace z3w {

// Storage: smallest unsigned integer type that fits W bits.
template <unsigned W>
using StorageType = std::conditional_t<
    (W <= 8), uint8_t,
    std::conditional_t<(W <= 16), uint16_t,
                       std::conditional_t<(W <= 32), uint32_t, uint64_t>>>;

template <unsigned W, bool IsSigned>
class Int {
  static_assert(W >= 1 && W <= 64, "Bit-width must be between 1 and 64.");

 public:
  static constexpr unsigned kWidth = W;
  static constexpr bool kIsSigned = IsSigned;
  using Storage = StorageType<W>;

  // Raw constructor: masks to W bits.
  explicit Int(uint64_t raw) : bits_(mask(raw)) {}

  // Compile-time range-checked literal.
  template <int64_t Value>
  static Int Literal() {
    if constexpr (IsSigned) {
      static_assert(Value >= min_signed() && Value <= max_signed(),
                    "Literal value does not fit in signed bit-width.");
      // Store as two's complement.
      return Int(static_cast<uint64_t>(Value));
    } else {
      static_assert(Value >= 0, "Unsigned literal must be non-negative.");
      static_assert(static_cast<uint64_t>(Value) <= max_unsigned(),
                    "Literal value does not fit in unsigned bit-width.");
      return Int(static_cast<uint64_t>(Value));
    }
  }

  // Runtime checked construction.
  static std::pair<Int, bool> checked(uint64_t raw) {
    Int result(raw);
    bool truncated = (raw != result.bits_);
    return {result, truncated};
  }

  // Access the underlying value (always unsigned representation).
  [[nodiscard]] Storage value() const { return bits_; }

 private:
  static constexpr Storage mask(uint64_t val) {
    if constexpr (W >= 64) {
      return static_cast<Storage>(val);
    } else {
      return static_cast<Storage>(val & ((uint64_t{1} << W) - 1));
    }
  }

  static constexpr int64_t min_signed() {
    if constexpr (W >= 64) {
      return INT64_MIN;
    } else {
      return -(int64_t{1} << (W - 1));
    }
  }

  static constexpr int64_t max_signed() {
    if constexpr (W >= 64) {
      return INT64_MAX;
    } else {
      return (int64_t{1} << (W - 1)) - 1;
    }
  }

  static constexpr uint64_t max_unsigned() {
    if constexpr (W >= 64) {
      return UINT64_MAX;
    } else {
      return (uint64_t{1} << W) - 1;
    }
  }

  Storage bits_;
};

template <unsigned W>
using UInt = Int<W, false>;

template <unsigned W>
using SInt = Int<W, true>;

}  // namespace z3w

#endif  // Z3WIRE_INT_H_
```

**Step 5: Run tests to verify they pass**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: PASS

**Step 6: Format and commit**

```bash
./dev.sh ./tools/format.sh
git add z3wire/int.h z3wire/int_test.cc z3wire/BUILD.bazel z3wire/CMakeLists.txt
git commit -m "Add Int<W,S> class with construction and storage"
```

---

### Task 2: Bitwise operators and equality

**Files:**
- Modify: `z3wire/int.h`
- Modify: `z3wire/int_test.cc`

**Step 1: Write failing tests**

Add to `z3wire/int_test.cc`:

```cpp
// --- Bitwise operators ---

TEST(IntTest, BitwiseAnd) {
  UInt<8> a(0xF0);
  UInt<8> b(0x3C);
  UInt<8> c = a & b;
  EXPECT_EQ(c.value(), 0x30);
}

TEST(IntTest, BitwiseOr) {
  UInt<8> a(0xF0);
  UInt<8> b(0x0F);
  UInt<8> c = a | b;
  EXPECT_EQ(c.value(), 0xFF);
}

TEST(IntTest, BitwiseXor) {
  UInt<8> a(0xFF);
  UInt<8> b(0x0F);
  UInt<8> c = a ^ b;
  EXPECT_EQ(c.value(), 0xF0);
}

TEST(IntTest, BitwiseNot) {
  UInt<8> a(0xF0);
  UInt<8> b = ~a;
  EXPECT_EQ(b.value(), 0x0F);
}

TEST(IntTest, BitwiseNotNonPowerOfTwo) {
  UInt<5> a(0x1F);
  UInt<5> b = ~a;
  EXPECT_EQ(b.value(), 0x00);  // ~0x1F masked to 5 bits
}

// --- Equality ---

TEST(IntTest, Equality) {
  UInt<8> a(42);
  UInt<8> b(42);
  UInt<8> c(99);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
}

TEST(IntTest, Inequality) {
  UInt<8> a(42);
  UInt<8> b(99);
  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a != a);
}
```

**Step 2: Run tests to verify they fail**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: FAIL (operators not defined)

**Step 3: Implement bitwise operators and equality**

Add inside the `Int` class in `z3wire/int.h` (after the `value()` method):

```cpp
  // --- Bitwise operators (strict: same width and signedness) ---

  friend Int operator&(const Int& lhs, const Int& rhs) {
    return Int(lhs.bits_ & rhs.bits_);
  }

  friend Int operator|(const Int& lhs, const Int& rhs) {
    return Int(lhs.bits_ | rhs.bits_);
  }

  friend Int operator^(const Int& lhs, const Int& rhs) {
    return Int(lhs.bits_ ^ rhs.bits_);
  }

  Int operator~() const { return Int(~bits_); }

  // --- Equality ---

  friend bool operator==(const Int& lhs, const Int& rhs) {
    return lhs.bits_ == rhs.bits_;
  }

  friend bool operator!=(const Int& lhs, const Int& rhs) {
    return lhs.bits_ != rhs.bits_;
  }
```

**Step 4: Run tests to verify they pass**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: PASS

**Step 5: Commit**

```bash
git add z3wire/int.h z3wire/int_test.cc
git commit -m "Add bitwise operators and equality to Int"
```

---

### Task 3: Ordered comparisons

**Files:**
- Modify: `z3wire/int.h`
- Modify: `z3wire/int_test.cc`

**Step 1: Write failing tests**

Add to `z3wire/int_test.cc`:

```cpp
// --- Ordered comparison (unsigned) ---

TEST(IntTest, UnsignedLessThan) {
  UInt<8> a(100);
  UInt<8> b(200);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
}

TEST(IntTest, UnsignedGreaterThan) {
  UInt<8> a(200);
  UInt<8> b(100);
  EXPECT_TRUE(a > b);
  EXPECT_FALSE(b > a);
}

TEST(IntTest, UnsignedLessEqual) {
  UInt<8> a(100);
  UInt<8> b(100);
  EXPECT_TRUE(a <= b);
}

TEST(IntTest, UnsignedGreaterEqual) {
  UInt<8> a(100);
  UInt<8> b(100);
  EXPECT_TRUE(a >= b);
}

// --- Ordered comparison (signed) ---

TEST(IntTest, SignedLessThan) {
  // 200 as SInt<8> is -56. So -56 < 100.
  SInt<8> a(200);  // -56
  SInt<8> b(100);
  EXPECT_TRUE(a < b);
}

TEST(IntTest, SignedGreaterThan) {
  SInt<8> a(100);
  SInt<8> b(200);  // -56
  EXPECT_TRUE(a > b);
}

TEST(IntTest, SignedComparisonNonPowerOfTwo) {
  // SInt<5>: range -16 to 15. Value 0x1F = 31 unsigned = -1 signed.
  SInt<5> a(0x1F);  // -1
  SInt<5> b(1);
  EXPECT_TRUE(a < b);
}
```

**Step 2: Run tests to verify they fail**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: FAIL

**Step 3: Implement ordered comparisons**

Add inside the `Int` class, after equality operators. For signed comparison, sign-extend the stored value to `int64_t`:

```cpp
  // --- Ordered comparison ---

  friend bool operator<(const Int& lhs, const Int& rhs) {
    if constexpr (IsSigned) {
      return sign_extend(lhs.bits_) < sign_extend(rhs.bits_);
    } else {
      return lhs.bits_ < rhs.bits_;
    }
  }

  friend bool operator<=(const Int& lhs, const Int& rhs) {
    if constexpr (IsSigned) {
      return sign_extend(lhs.bits_) <= sign_extend(rhs.bits_);
    } else {
      return lhs.bits_ <= rhs.bits_;
    }
  }

  friend bool operator>(const Int& lhs, const Int& rhs) {
    if constexpr (IsSigned) {
      return sign_extend(lhs.bits_) > sign_extend(rhs.bits_);
    } else {
      return lhs.bits_ > rhs.bits_;
    }
  }

  friend bool operator>=(const Int& lhs, const Int& rhs) {
    if constexpr (IsSigned) {
      return sign_extend(lhs.bits_) >= sign_extend(rhs.bits_);
    } else {
      return lhs.bits_ >= rhs.bits_;
    }
  }
```

Add `sign_extend` as a private static helper:

```cpp
  static constexpr int64_t sign_extend(Storage val) {
    if constexpr (W >= 64) {
      return static_cast<int64_t>(val);
    } else {
      // If the sign bit is set, extend with 1s.
      uint64_t sign_bit = uint64_t{1} << (W - 1);
      return static_cast<int64_t>((static_cast<uint64_t>(val) ^ sign_bit) -
                                  sign_bit);
    }
  }
```

**Step 4: Run tests to verify they pass**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: PASS

**Step 5: Commit**

```bash
git add z3wire/int.h z3wire/int_test.cc
git commit -m "Add ordered comparisons to Int with sign-aware dispatch"
```

---

### Task 4: Bit-growth arithmetic

**Files:**
- Modify: `z3wire/int.h`
- Modify: `z3wire/int_test.cc`

**Step 1: Write failing tests**

Add to `z3wire/int_test.cc`:

```cpp
// --- Arithmetic with bit growth ---

TEST(IntTest, AdditionWidens) {
  UInt<8> a(255);
  UInt<8> b(255);
  auto sum = a + b;
  static_assert(decltype(sum)::kWidth == 9);
  static_assert(!decltype(sum)::kIsSigned);
  EXPECT_EQ(sum.value(), 510);
}

TEST(IntTest, AdditionDifferentWidths) {
  UInt<8> a(255);
  UInt<4> b(15);
  auto sum = a + b;
  static_assert(decltype(sum)::kWidth == 9);
  EXPECT_EQ(sum.value(), 270);
}

TEST(IntTest, AdditionMixedSignedness) {
  UInt<8> a(100);
  SInt<8> b(200);  // -56 signed
  auto sum = a + b;
  static_assert(decltype(sum)::kWidth == 9);
  static_assert(decltype(sum)::kIsSigned);
  // 100 + (-56) = 44
  EXPECT_EQ(Int<9, true>::Literal<44>().value(), sum.value());
}

TEST(IntTest, SubtractionIsSigned) {
  UInt<8> a(100);
  UInt<8> b(200);
  auto diff = a - b;
  static_assert(decltype(diff)::kWidth == 9);
  static_assert(decltype(diff)::kIsSigned);
  // 100 - 200 = -100. In 9-bit two's complement: 512 - 100 = 412.
  EXPECT_EQ(diff.value(), 412);
}

TEST(IntTest, SubtractionPositiveResult) {
  UInt<8> a(200);
  UInt<8> b(100);
  auto diff = a - b;
  EXPECT_EQ(diff.value(), 100);
}
```

**Step 2: Run tests to verify they fail**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: FAIL

**Step 3: Implement bit-growth arithmetic**

Add as free functions after the `Int` class in `z3wire/int.h`, before the `UInt`/`SInt` aliases. Use a helper to extend concrete values:

```cpp
namespace internal {

template <unsigned TargetW, unsigned SrcW, bool SrcS>
uint64_t extend(const Int<SrcW, SrcS>& val) {
  if constexpr (!SrcS) {
    // Unsigned: zero-extend (value is already correct).
    return val.value();
  } else {
    // Signed: sign-extend to 64 bits, then mask to target width.
    int64_t signed_val = Int<SrcW, SrcS>::sign_extend_value(val.value());
    if constexpr (TargetW >= 64) {
      return static_cast<uint64_t>(signed_val);
    } else {
      return static_cast<uint64_t>(signed_val) & ((uint64_t{1} << TargetW) - 1);
    }
  }
}

}  // namespace internal

// Addition: result width = max(W1, W2) + 1.
template <unsigned W1, bool S1, unsigned W2, bool S2>
auto operator+(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  constexpr unsigned kResultWidth = std::max(W1, W2) + 1;
  constexpr bool kResultSigned = S1 || S2;
  uint64_t lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  uint64_t rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return Int<kResultWidth, kResultSigned>(lhs_ext + rhs_ext);
}

// Subtraction: result width = max(W1, W2) + 1, always signed.
template <unsigned W1, bool S1, unsigned W2, bool S2>
auto operator-(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  constexpr unsigned kResultWidth = std::max(W1, W2) + 1;
  uint64_t lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  uint64_t rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return Int<kResultWidth, true>(lhs_ext - rhs_ext);
}
```

Note: `sign_extend` needs to be exposed. Make a public static method `sign_extend_value` (or make the `internal` namespace a friend). The simplest approach is to add to `Int`:

```cpp
  // Public for use by internal::extend.
  static constexpr int64_t sign_extend_value(Storage val) {
    return sign_extend(val);
  }
```

Also add `#include <algorithm>` to `int.h` for `std::max`.

**Step 4: Run tests to verify they pass**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: PASS

**Step 5: Commit**

```bash
git add z3wire/int.h z3wire/int_test.cc
git commit -m "Add bit-growth arithmetic (+, -) to Int"
```

---

### Task 5: Hardware shifts and checked shifts

**Files:**
- Modify: `z3wire/int.h`
- Modify: `z3wire/int_test.cc`

**Step 1: Write failing tests**

Add to `z3wire/int_test.cc`:

```cpp
// --- Hardware shifts ---

TEST(IntTest, LeftShift) {
  UInt<8> a(1);
  UInt<8> n(4);
  UInt<8> result = a << n;
  EXPECT_EQ(result.value(), 16);
}

TEST(IntTest, LeftShiftOverflow) {
  UInt<8> a(0x80);
  UInt<8> n(1);
  UInt<8> result = a << n;
  EXPECT_EQ(result.value(), 0);  // Bit shifted out
}

TEST(IntTest, UnsignedRightShift) {
  UInt<8> a(0x80);
  UInt<8> n(4);
  UInt<8> result = a >> n;
  EXPECT_EQ(result.value(), 0x08);  // Logical shift
}

TEST(IntTest, SignedRightShift) {
  SInt<8> a(0x80);  // -128
  SInt<8> n(4);
  SInt<8> result = a >> n;
  EXPECT_EQ(result.value(), 0xF8);  // Arithmetic shift: sign-extended
}

// --- Checked shifts ---

TEST(IntTest, CheckedShlNoLoss) {
  UInt<8> a(1);
  UInt<8> n(4);
  auto [shifted, lost] = checked_shl(a, n);
  EXPECT_EQ(shifted.value(), 16);
  EXPECT_FALSE(lost);
}

TEST(IntTest, CheckedShlWithLoss) {
  UInt<8> a(0x80);
  UInt<8> n(1);
  auto [shifted, lost] = checked_shl(a, n);
  EXPECT_EQ(shifted.value(), 0);
  EXPECT_TRUE(lost);
}

TEST(IntTest, CheckedShrWithLoss) {
  UInt<8> a(1);
  UInt<8> n(1);
  auto [shifted, lost] = checked_shr(a, n);
  EXPECT_EQ(shifted.value(), 0);
  EXPECT_TRUE(lost);
}

// --- Lossless left shift ---

TEST(IntTest, LosslessShlConstant) {
  UInt<8> a(0xFF);
  auto result = lossless_shl<3>(a);
  static_assert(decltype(result)::kWidth == 11);
  EXPECT_EQ(result.value(), 0x7F8);
}
```

**Step 2: Run tests to verify they fail**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: FAIL

**Step 3: Implement shifts**

Add inside `Int` class for hardware shifts:

```cpp
  // --- Hardware shifts (strict: same width and signedness) ---

  friend Int operator<<(const Int& lhs, const Int& rhs) {
    return Int(static_cast<uint64_t>(lhs.bits_) << rhs.bits_);
  }

  friend Int operator>>(const Int& lhs, const Int& rhs) {
    if constexpr (IsSigned) {
      // Arithmetic right shift: sign-extend, shift, re-mask.
      int64_t signed_val = sign_extend(lhs.bits_);
      return Int(static_cast<uint64_t>(signed_val >> rhs.bits_));
    } else {
      return Int(lhs.bits_ >> rhs.bits_);
    }
  }
```

Add as free functions for checked and lossless shifts:

```cpp
// checked_shl: returns {shifted, lost}.
template <unsigned W, bool S>
std::pair<Int<W, S>, bool> checked_shl(const Int<W, S>& val,
                                       const Int<W, S>& amount) {
  auto shifted = val << amount;
  auto restored = shifted >> amount;
  bool lost = (restored != val);
  return {shifted, lost};
}

// checked_shr: returns {shifted, lost}.
template <unsigned W, bool S>
std::pair<Int<W, S>, bool> checked_shr(const Int<W, S>& val,
                                       const Int<W, S>& amount) {
  auto shifted = val >> amount;
  auto restored = shifted << amount;
  bool lost = (restored != val);
  return {shifted, lost};
}

// Lossless left shift by constant N: result width = W + N.
template <unsigned N, unsigned W, bool S>
UInt<W + N> lossless_shl(const Int<W, S>& val) {
  return UInt<W + N>(static_cast<uint64_t>(val.value()) << N);
}
```

**Step 4: Run tests to verify they pass**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: PASS

**Step 5: Commit**

```bash
git add z3wire/int.h z3wire/int_test.cc
git commit -m "Add hardware, checked, and lossless shifts to Int"
```

---

### Task 6: Casting (three tiers)

**Files:**
- Modify: `z3wire/int.h`
- Modify: `z3wire/int_test.cc`

**Step 1: Write failing tests**

Add to `z3wire/int_test.cc`:

```cpp
// --- cast ---

TEST(IntTest, CastTruncation) {
  UInt<16> a(0x1234);
  auto b = cast<UInt<8>>(a);
  static_assert(decltype(b)::kWidth == 8);
  EXPECT_EQ(b.value(), 0x34);
}

TEST(IntTest, CastZeroExtension) {
  UInt<8> a(0xFF);
  auto b = cast<UInt<16>>(a);
  static_assert(decltype(b)::kWidth == 16);
  EXPECT_EQ(b.value(), 0x00FF);
}

TEST(IntTest, CastSignExtension) {
  SInt<8> a(0x80);  // -128
  auto b = cast<SInt<16>>(a);
  EXPECT_EQ(b.value(), 0xFF80);
}

TEST(IntTest, CastBitcast) {
  UInt<8> a(42);
  auto b = cast<SInt<8>>(a);
  static_assert(decltype(b)::kIsSigned);
  EXPECT_EQ(b.value(), 42);
}

// --- safe_cast ---

TEST(IntTest, SafeCastWidening) {
  UInt<8> a(255);
  auto b = safe_cast<UInt<16>>(a);
  static_assert(decltype(b)::kWidth == 16);
  EXPECT_EQ(b.value(), 255);
}

TEST(IntTest, SafeCastUnsignedToSigned) {
  UInt<8> a(255);
  auto b = safe_cast<SInt<9>>(a);
  static_assert(decltype(b)::kWidth == 9);
  EXPECT_EQ(b.value(), 255);
}

// --- checked_cast ---

TEST(IntTest, CheckedCastNoOverflow) {
  UInt<16> a(42);
  auto [result, overflowed] = checked_cast<UInt<8>>(a);
  EXPECT_EQ(result.value(), 42);
  EXPECT_FALSE(overflowed);
}

TEST(IntTest, CheckedCastWithOverflow) {
  UInt<16> a(256);
  auto [result, overflowed] = checked_cast<UInt<8>>(a);
  EXPECT_EQ(result.value(), 0);
  EXPECT_TRUE(overflowed);
}
```

**Step 2: Run tests to verify they fail**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: FAIL

**Step 3: Implement casting**

Add as free functions in `z3wire/int.h`. These overload the same names as the symbolic versions; C++ overload resolution picks the right one based on argument type:

```cpp
// cast<T>(val): raw hardware cast.
template <typename Target, unsigned SrcW, bool SrcS>
Target cast(const Int<SrcW, SrcS>& val) {
  constexpr unsigned kTgtW = Target::kWidth;
  if constexpr (kTgtW == SrcW) {
    return Target(val.value());
  } else if constexpr (kTgtW < SrcW) {
    return Target(val.value());  // Constructor masks
  } else {
    if constexpr (SrcS) {
      // Sign-extend.
      int64_t signed_val = Int<SrcW, SrcS>::sign_extend_value(val.value());
      return Target(static_cast<uint64_t>(signed_val));
    } else {
      return Target(val.value());  // Zero-extend (already correct)
    }
  }
}

// safe_cast<T>(val): compile-time lossless check.
template <typename Target, unsigned SrcW, bool SrcS>
Target safe_cast(const Int<SrcW, SrcS>& val) {
  constexpr unsigned kTgtW = Target::kWidth;
  constexpr bool kTgtS = Target::kIsSigned;

  static_assert(!SrcS || kTgtS,
                "safe_cast from signed to unsigned is always forbidden.");

  if constexpr (!SrcS && !kTgtS) {
    static_assert(kTgtW >= SrcW,
                  "safe_cast: target width too small for lossless conversion.");
  } else if constexpr (SrcS && kTgtS) {
    static_assert(kTgtW >= SrcW,
                  "safe_cast: target width too small for lossless conversion.");
  } else if constexpr (!SrcS && kTgtS) {
    static_assert(kTgtW > SrcW,
                  "safe_cast: unsigned-to-signed needs target width > source "
                  "width.");
  }

  return cast<Target>(val);
}

// checked_cast<T>(val): returns {result, overflowed}.
template <typename Target, unsigned SrcW, bool SrcS>
std::pair<Target, bool> checked_cast(const Int<SrcW, SrcS>& val) {
  auto result = cast<Target>(val);
  auto roundtrip = cast<Int<SrcW, SrcS>>(result);
  bool overflowed = (roundtrip != val);
  return {result, overflowed};
}
```

**Step 4: Run tests to verify they pass**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: PASS

**Step 5: Commit**

```bash
git add z3wire/int.h z3wire/int_test.cc
git commit -m "Add three-tier casting API to Int"
```

---

### Task 7: Bit manipulation (extract, concat, bool conversion, ite)

**Files:**
- Modify: `z3wire/int.h`
- Modify: `z3wire/int_test.cc`

**Step 1: Write failing tests**

Add to `z3wire/int_test.cc`:

```cpp
// --- extract ---

TEST(IntTest, StaticExtract) {
  UInt<16> a(0x1234);
  auto high = extract<15, 8>(a);
  auto low = extract<7, 0>(a);
  static_assert(decltype(high)::kWidth == 8);
  static_assert(decltype(low)::kWidth == 8);
  EXPECT_EQ(high.value(), 0x12);
  EXPECT_EQ(low.value(), 0x34);
}

// --- concat ---

TEST(IntTest, Concat) {
  UInt<8> high(0xAB);
  UInt<8> low(0xCD);
  auto full = concat(high, low);
  static_assert(decltype(full)::kWidth == 16);
  EXPECT_EQ(full.value(), 0xABCD);
}

TEST(IntTest, ConcatVariadic) {
  UInt<4> a(0xA);
  UInt<4> b(0xB);
  UInt<4> c(0xC);
  auto result = concat(a, b, c);
  static_assert(decltype(result)::kWidth == 12);
  EXPECT_EQ(result.value(), 0xABC);
}

// --- Bool / UInt<1> conversion ---

TEST(IntTest, ToUInt1) {
  auto one = to_uint1(true);
  auto zero = to_uint1(false);
  EXPECT_EQ(one.value(), 1);
  EXPECT_EQ(zero.value(), 0);
}

TEST(IntTest, ToBoolFromUInt1) {
  UInt<1> one(1);
  UInt<1> zero(0);
  EXPECT_TRUE(to_bool(one));
  EXPECT_FALSE(to_bool(zero));
}

// --- ite ---

TEST(IntTest, IteTrue) {
  UInt<8> a(42);
  UInt<8> b(99);
  UInt<8> result = ite(true, a, b);
  EXPECT_EQ(result.value(), 42);
}

TEST(IntTest, IteFalse) {
  UInt<8> a(42);
  UInt<8> b(99);
  UInt<8> result = ite(false, a, b);
  EXPECT_EQ(result.value(), 99);
}
```

**Step 2: Run tests to verify they fail**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: FAIL

**Step 3: Implement bit manipulation**

Add as free functions in `z3wire/int.h`:

```cpp
// Static extract: extract<High, Low>(val) -> UInt<High - Low + 1>.
template <unsigned High, unsigned Low, unsigned W, bool S>
UInt<High - Low + 1> extract(const Int<W, S>& val) {
  static_assert(High >= Low, "extract: High must be >= Low.");
  static_assert(High < W, "extract: High must be < input width.");
  return UInt<High - Low + 1>(val.value() >> Low);
}

// concat(a, b): result width = W1 + W2, always UInt.
template <unsigned W1, bool S1, unsigned W2, bool S2>
UInt<W1 + W2> concat(const Int<W1, S1>& high, const Int<W2, S2>& low) {
  uint64_t result = (static_cast<uint64_t>(high.value()) << W2) |
                    static_cast<uint64_t>(low.value());
  return UInt<W1 + W2>(result);
}

// Variadic concat.
template <unsigned W1, bool S1, unsigned W2, bool S2, typename... Rest>
auto concat(const Int<W1, S1>& high, const Int<W2, S2>& next,
            const Rest&... rest) {
  return concat(concat(high, next), rest...);
}

// Bool / UInt<1> conversion.
inline UInt<1> to_uint1(bool b) { return UInt<1>(b ? 1 : 0); }

inline bool to_bool(const UInt<1>& v) { return v.value() != 0; }

// ite: concrete conditional selection.
template <unsigned W, bool S>
Int<W, S> ite(bool cond, const Int<W, S>& true_val,
              const Int<W, S>& false_val) {
  return cond ? true_val : false_val;
}
```

**Step 4: Run tests to verify they pass**

Run: `./dev.sh bazel test //z3wire:int_test`
Expected: PASS

**Step 5: Commit**

```bash
git add z3wire/int.h z3wire/int_test.cc
git commit -m "Add extract, concat, bool conversion, and ite to Int"
```

---

### Task 8: Type traits and promotion helpers

**Files:**
- Modify: `z3wire/int.h`
- Modify: `z3wire/bitvec.h`

This task adds the type traits (`is_concrete_v`, `is_symbolic_v`) and the `to_symbolic` helper that converts a concrete `Int` to a symbolic `BitVec`. These are needed before adding mixed overloads.

**Step 1: Write failing tests**

Add to `z3wire/int_test.cc`:

```cpp
// --- Type traits ---

TEST(IntTest, IsConcreteV) {
  static_assert(is_concrete_v<UInt<8>>);
  static_assert(is_concrete_v<SInt<16>>);
  static_assert(!is_concrete_v<int>);
}
```

Add to `z3wire/bitvec_test.cc`:

```cpp
// --- Type traits ---

TEST_F(BitVecTest, IsSymbolicV) {
  static_assert(is_symbolic_v<Ubv<8>>);
  static_assert(is_symbolic_v<Sbv<16>>);
  static_assert(!is_symbolic_v<int>);
}

// --- Concrete to symbolic promotion ---

TEST_F(BitVecTest, ConcreteToSymbolic) {
  UInt<8> concrete(42);
  Ubv<8> symbolic = to_symbolic(concrete, ctx_);

  z3::solver s(ctx_);
  s.add(symbolic.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, SignedConcreteToSymbolic) {
  SInt<8> concrete(0x80);  // -128
  Sbv<8> symbolic = to_symbolic(concrete, ctx_);

  z3::solver s(ctx_);
  s.add(symbolic.raw() != ctx_.bv_val(0x80, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}
```

**Step 2: Run tests to verify they fail**

Run: `./dev.sh bazel test //z3wire:int_test //z3wire:bitvec_test`
Expected: FAIL

**Step 3: Implement type traits and promotion**

Add to `z3wire/int.h` (after the `UInt`/`SInt` aliases):

```cpp
// Type trait: is this a concrete Int type?
template <typename T>
struct is_concrete : std::false_type {};

template <unsigned W, bool S>
struct is_concrete<Int<W, S>> : std::true_type {};

template <typename T>
inline constexpr bool is_concrete_v = is_concrete<T>::value;
```

Add to `z3wire/bitvec.h` (after the `Ubv`/`Sbv` aliases, and add `#include "z3wire/int.h"`):

```cpp
// Type trait: is this a symbolic BitVec type?
template <typename T>
struct is_symbolic : std::false_type {};

template <size_t W, bool S>
struct is_symbolic<BitVec<W, S>> : std::true_type {};

template <typename T>
inline constexpr bool is_symbolic_v = is_symbolic<T>::value;

// Convert a concrete Int to a symbolic BitVec.
template <unsigned W, bool S>
BitVec<W, S> to_symbolic(const Int<W, S>& val, z3::context& ctx) {
  return BitVec<W, S>(ctx.bv_val(static_cast<uint64_t>(val.value()), W));
}
```

**Step 4: Run tests to verify they pass**

Run: `./dev.sh bazel test //z3wire:int_test //z3wire:bitvec_test`
Expected: PASS

**Step 5: Commit**

```bash
git add z3wire/int.h z3wire/bitvec.h z3wire/int_test.cc z3wire/bitvec_test.cc
git commit -m "Add type traits and concrete-to-symbolic promotion"
```

---

### Task 9: Mixed concrete+symbolic arithmetic overloads

**Files:**
- Modify: `z3wire/bitvec.h`
- Modify: `z3wire/bitvec_test.cc`

**Step 1: Write failing tests**

Add to `z3wire/bitvec_test.cc`:

```cpp
// --- Mixed concrete + symbolic arithmetic ---

TEST_F(BitVecTest, MixedAddSymbolicPlusConcrete) {
  Ubv<8> sym(ctx_, "x");
  UInt<8> conc(42);
  auto result = sym + conc;

  static_assert(decltype(result)::kWidth == 9);
  static_assert(is_symbolic_v<decltype(result)>);

  z3::solver s(ctx_);
  s.add(sym.raw() == ctx_.bv_val(10, 8));
  s.add(result.raw() != ctx_.bv_val(52, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, MixedAddConcretePlusSymbolic) {
  UInt<8> conc(42);
  Ubv<8> sym(ctx_, "x");
  auto result = conc + sym;

  static_assert(decltype(result)::kWidth == 9);
  static_assert(is_symbolic_v<decltype(result)>);
}

TEST_F(BitVecTest, MixedSubSymbolicMinusConcrete) {
  Ubv<8> sym(ctx_, "x");
  UInt<8> conc(50);
  auto result = sym - conc;

  static_assert(decltype(result)::kWidth == 9);
  static_assert(decltype(result)::kIsSigned);
  static_assert(is_symbolic_v<decltype(result)>);
}

TEST_F(BitVecTest, MixedAddDifferentWidths) {
  Ubv<8> sym(ctx_, "x");
  UInt<4> conc(15);
  auto result = sym + conc;

  static_assert(decltype(result)::kWidth == 9);
}
```

**Step 2: Run tests to verify they fail**

Run: `./dev.sh bazel test //z3wire:bitvec_test`
Expected: FAIL

**Step 3: Implement mixed arithmetic overloads**

Add to `z3wire/bitvec.h` (after the existing `operator-` for symbolic):

```cpp
// --- Mixed concrete + symbolic arithmetic ---

// Helper: get context from a symbolic value.
namespace internal {

template <size_t W, bool S>
z3::context& get_context(const BitVec<W, S>& val) {
  return val.raw().ctx();
}

}  // namespace internal

// Addition: symbolic + concrete.
template <size_t W1, bool S1, unsigned W2, bool S2>
auto operator+(const BitVec<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  auto& ctx = internal::get_context(lhs);
  return lhs + to_symbolic(rhs, ctx);
}

// Addition: concrete + symbolic.
template <unsigned W1, bool S1, size_t W2, bool S2>
auto operator+(const Int<W1, S1>& lhs, const BitVec<W2, S2>& rhs) {
  auto& ctx = internal::get_context(rhs);
  return to_symbolic(lhs, ctx) + rhs;
}

// Subtraction: symbolic - concrete.
template <size_t W1, bool S1, unsigned W2, bool S2>
auto operator-(const BitVec<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  auto& ctx = internal::get_context(lhs);
  return lhs - to_symbolic(rhs, ctx);
}

// Subtraction: concrete - symbolic.
template <unsigned W1, bool S1, size_t W2, bool S2>
auto operator-(const Int<W1, S1>& lhs, const BitVec<W2, S2>& rhs) {
  auto& ctx = internal::get_context(rhs);
  return to_symbolic(lhs, ctx) - rhs;
}
```

**Step 4: Run tests to verify they pass**

Run: `./dev.sh bazel test //z3wire:bitvec_test`
Expected: PASS

**Step 5: Commit**

```bash
git add z3wire/bitvec.h z3wire/bitvec_test.cc
git commit -m "Add mixed concrete+symbolic arithmetic overloads"
```

---

### Task 10: Mixed bitwise, comparison, and ite overloads

**Files:**
- Modify: `z3wire/bitvec.h`
- Modify: `z3wire/bitvec_test.cc`

**Step 1: Write failing tests**

Add to `z3wire/bitvec_test.cc`:

```cpp
// --- Mixed bitwise ---

TEST_F(BitVecTest, MixedBitwiseAnd) {
  Ubv<8> sym(ctx_, "x");
  UInt<8> conc(0x0F);
  Ubv<8> result = sym & conc;

  z3::solver s(ctx_);
  s.add(sym.raw() == ctx_.bv_val(0xAB, 8));
  s.add(result.raw() != ctx_.bv_val(0x0B, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed comparison ---

TEST_F(BitVecTest, MixedEquality) {
  Ubv<8> sym(ctx_, "x");
  UInt<8> conc(42);
  Bool eq = (sym == conc);

  z3::solver s(ctx_);
  s.add(eq.raw());
  s.add(sym.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, MixedLessThan) {
  Ubv<8> sym(ctx_, "x");
  UInt<8> conc(100);
  Bool lt = (sym < conc);

  z3::solver s(ctx_);
  s.add(lt.raw());
  s.add(sym.raw() == ctx_.bv_val(200, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed ite ---

TEST_F(BitVecTest, MixedIteSymbolicCondConcreteValues) {
  Bool cond(ctx_, "c");
  UInt<8> a(42);
  UInt<8> b(99);
  auto result = ite(cond, a, b);

  static_assert(is_symbolic_v<decltype(result)>);

  z3::solver s(ctx_);
  s.add(cond.raw());
  s.add(result.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, MixedIteSymbolicCondMixedValues) {
  Bool cond(ctx_, "c");
  Ubv<8> sym(ctx_, "x");
  UInt<8> conc(99);
  auto result = ite(cond, sym, conc);

  static_assert(is_symbolic_v<decltype(result)>);
}
```

**Step 2: Run tests to verify they fail**

Run: `./dev.sh bazel test //z3wire:bitvec_test`
Expected: FAIL

**Step 3: Implement mixed bitwise, comparison, and ite**

Add to `z3wire/bitvec.h`. For bitwise and comparison, follow the same pattern as arithmetic (promote concrete, delegate to existing symbolic operator). For ite, use the concept-constrained approach:

```cpp
// --- Mixed bitwise operators ---

// Helper macro pattern: sym OP conc and conc OP sym for each bitwise op.
template <size_t W, bool S>
BitVec<W, S> operator&(const BitVec<W, S>& lhs, const Int<W, S>& rhs) {
  return lhs & to_symbolic(rhs, lhs.raw().ctx());
}

template <size_t W, bool S>
BitVec<W, S> operator&(const Int<W, S>& lhs, const BitVec<W, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) & rhs;
}

template <size_t W, bool S>
BitVec<W, S> operator|(const BitVec<W, S>& lhs, const Int<W, S>& rhs) {
  return lhs | to_symbolic(rhs, lhs.raw().ctx());
}

template <size_t W, bool S>
BitVec<W, S> operator|(const Int<W, S>& lhs, const BitVec<W, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) | rhs;
}

template <size_t W, bool S>
BitVec<W, S> operator^(const BitVec<W, S>& lhs, const Int<W, S>& rhs) {
  return lhs ^ to_symbolic(rhs, lhs.raw().ctx());
}

template <size_t W, bool S>
BitVec<W, S> operator^(const Int<W, S>& lhs, const BitVec<W, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) ^ rhs;
}

// --- Mixed comparison operators ---

template <size_t W, bool S>
Bool operator==(const BitVec<W, S>& lhs, const Int<W, S>& rhs) {
  return lhs == to_symbolic(rhs, lhs.raw().ctx());
}

template <size_t W, bool S>
Bool operator==(const Int<W, S>& lhs, const BitVec<W, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) == rhs;
}

template <size_t W, bool S>
Bool operator!=(const BitVec<W, S>& lhs, const Int<W, S>& rhs) {
  return lhs != to_symbolic(rhs, lhs.raw().ctx());
}

template <size_t W, bool S>
Bool operator!=(const Int<W, S>& lhs, const BitVec<W, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) != rhs;
}

template <size_t W, bool S>
Bool operator<(const BitVec<W, S>& lhs, const Int<W, S>& rhs) {
  return lhs < to_symbolic(rhs, lhs.raw().ctx());
}

template <size_t W, bool S>
Bool operator<(const Int<W, S>& lhs, const BitVec<W, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) < rhs;
}

template <size_t W, bool S>
Bool operator<=(const BitVec<W, S>& lhs, const Int<W, S>& rhs) {
  return lhs <= to_symbolic(rhs, lhs.raw().ctx());
}

template <size_t W, bool S>
Bool operator<=(const Int<W, S>& lhs, const BitVec<W, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) <= rhs;
}

template <size_t W, bool S>
Bool operator>(const BitVec<W, S>& lhs, const Int<W, S>& rhs) {
  return lhs > to_symbolic(rhs, lhs.raw().ctx());
}

template <size_t W, bool S>
Bool operator>(const Int<W, S>& lhs, const BitVec<W, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) > rhs;
}

template <size_t W, bool S>
Bool operator>=(const BitVec<W, S>& lhs, const Int<W, S>& rhs) {
  return lhs >= to_symbolic(rhs, lhs.raw().ctx());
}

template <size_t W, bool S>
Bool operator>=(const Int<W, S>& lhs, const BitVec<W, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) >= rhs;
}

// --- Mixed ite ---

// Bool condition with concrete values.
template <unsigned W, bool S>
BitVec<W, S> ite(const Bool& cond, const Int<W, S>& true_val,
                 const Int<W, S>& false_val) {
  auto& ctx = cond.raw().ctx();
  return ite(cond, to_symbolic(true_val, ctx), to_symbolic(false_val, ctx));
}

// Bool condition with mixed values.
template <size_t W, bool S>
BitVec<W, S> ite(const Bool& cond, const BitVec<W, S>& true_val,
                 const Int<W, S>& false_val) {
  auto& ctx = cond.raw().ctx();
  return ite(cond, true_val, to_symbolic(false_val, ctx));
}

template <size_t W, bool S>
BitVec<W, S> ite(const Bool& cond, const Int<W, S>& true_val,
                 const BitVec<W, S>& false_val) {
  auto& ctx = cond.raw().ctx();
  return ite(cond, to_symbolic(true_val, ctx), false_val);
}

// bool condition with symbolic values (pick at C++ level).
template <size_t W, bool S>
BitVec<W, S> ite(bool cond, const BitVec<W, S>& true_val,
                 const BitVec<W, S>& false_val) {
  return cond ? true_val : false_val;
}
```

Note: the mixed `ite` uses explicit overloads rather than the single-concept approach we discussed. This is because the concept approach may have ambiguity with the existing concrete `ite` and the friend operators inside `BitVec`. Explicit overloads are clearer and the count (4 new overloads) is manageable.

**Step 4: Run tests to verify they pass**

Run: `./dev.sh bazel test //z3wire:bitvec_test`
Expected: PASS

**Step 5: Commit**

```bash
git add z3wire/bitvec.h z3wire/bitvec_test.cc
git commit -m "Add mixed concrete+symbolic bitwise, comparison, and ite overloads"
```

---

### Task 11: Run full test suite, format, and lint

**Files:**
- Potentially any file modified above

**Step 1: Format all files**

Run: `./dev.sh ./tools/format.sh`

**Step 2: Run linter**

Run: `./dev.sh ./tools/lint.sh`

Fix any issues found.

**Step 3: Run all tests**

Run: `./dev.sh bazel test //...`
Expected: ALL PASS

**Step 4: Commit any formatting/lint fixes**

```bash
git add -u
git commit -m "Fix formatting and lint issues"
```

---

Plan complete and saved to `docs/plans/2026-03-08-concrete-types.md`. Two execution options:

**1. Subagent-Driven (this session)** — I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Parallel Session (separate)** — Open a new session with executing-plans, batch execution with checkpoints

Which approach?