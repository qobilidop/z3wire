#ifndef Z3WIRE_BIT_VEC_H_
#define Z3WIRE_BIT_VEC_H_

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>

namespace z3w {

// Storage: smallest unsigned integer type that fits W bits.
template <size_t W>
using UnsignedStorageType = std::conditional_t<
    (W <= 8), uint8_t,
    std::conditional_t<(W <= 16), uint16_t,
                       std::conditional_t<(W <= 32), uint32_t, uint64_t>>>;

// Storage: smallest signed integer type that fits W bits.
template <size_t W>
using SignedStorageType = std::conditional_t<
    (W <= 8), int8_t,
    std::conditional_t<(W <= 16), int16_t,
                       std::conditional_t<(W <= 32), int32_t, int64_t>>>;

template <size_t W, bool IsSigned>
class BitVec {
  static_assert(W >= 1 && W <= 64, "Bit-width must be between 1 and 64.");

 public:
  static constexpr size_t kWidth = W;
  static constexpr bool kIsSigned = IsSigned;
  using Storage = std::conditional_t<IsSigned, SignedStorageType<W>,
                                     UnsignedStorageType<W>>;

  // Compile-time range-checked literal.
  template <int64_t Value>
  static constexpr BitVec Literal() {
    if constexpr (IsSigned) {
      static_assert(Value >= min_signed() && Value <= max_signed(),
                    "Literal value does not fit in bit-width.");
      return BitVec(static_cast<uint64_t>(Value));
    } else {
      static_assert(Value >= 0, "Unsigned literal must be non-negative.");
      static_assert(static_cast<uint64_t>(Value) <= max_unsigned(),
                    "Literal value does not fit in bit-width.");
      return BitVec(static_cast<uint64_t>(Value));
    }
  }

  // Runtime checked construction. Returns {value, truncated}.
  // Accepts any integer type; no implicit conversions.
  template <std::integral T>
  [[nodiscard]] static std::tuple<BitVec, bool> Checked(T raw) {
    if constexpr (IsSigned) {
      auto val = static_cast<int64_t>(raw);
      BitVec result(static_cast<uint64_t>(val));
      bool truncated = (val < min_signed() || val > max_signed());
      return {result, truncated};
    } else {
      if constexpr (std::is_signed_v<T>) {
        if (raw < 0) {
          return {BitVec(static_cast<uint64_t>(raw)), true};
        }
      }
      auto val = static_cast<uint64_t>(raw);
      BitVec result(val);
      bool truncated = (val != result.bits_);
      return {result, truncated};
    }
  }

  // Access the stored value.
  // Returns unsigned type for UInt, signed type for SInt.
  [[nodiscard]] constexpr Storage value() const { return bits_; }

  // Default constructor: zero-initializes.
  constexpr BitVec() : bits_(0) {}

 private:
  // Raw constructor: masks to W bits. Private to prevent silent truncation.
  constexpr explicit BitVec(uint64_t raw) : bits_(mask(raw)) {}

  static constexpr Storage mask(uint64_t val) {
    if constexpr (W >= 64) {
      return static_cast<Storage>(val);
    } else if constexpr (IsSigned) {
      // Mask to W bits, then sign-extend from W to storage width.
      uint64_t masked = val & ((uint64_t{1} << W) - 1);
      uint64_t sign_bit = uint64_t{1} << (W - 1);
      int64_t sign_extended =
          static_cast<int64_t>((masked ^ sign_bit) - sign_bit);
      return static_cast<Storage>(sign_extended);
    } else {
      return static_cast<Storage>(val & ((uint64_t{1} << W) - 1));
    }
  }

  static constexpr int64_t min_signed() {
    if constexpr (W >= 64) {
      return std::numeric_limits<int64_t>::min();
    } else {
      return -(int64_t{1} << (W - 1));
    }
  }

  static constexpr int64_t max_signed() {
    if constexpr (W >= 64) {
      return std::numeric_limits<int64_t>::max();
    } else {
      return (int64_t{1} << (W - 1)) - 1;
    }
  }

  static constexpr uint64_t max_unsigned() {
    if constexpr (W >= 64) {
      return std::numeric_limits<uint64_t>::max();
    } else {
      return (uint64_t{1} << W) - 1;
    }
  }

  Storage bits_;
};

template <size_t W>
using UInt = BitVec<W, false>;

template <size_t W>
using SInt = BitVec<W, true>;

// Type trait: is this a concrete BitVec type?
template <typename T>
struct is_concrete : std::false_type {};

template <size_t W, bool S>
struct is_concrete<BitVec<W, S>> : std::true_type {};

// Forward declaration for concrete Bool.
class Bool;

template <>
struct is_concrete<Bool> : std::true_type {};

template <typename T>
inline constexpr bool is_concrete_v = is_concrete<T>::value;

namespace internal {

template <size_t TargetW, size_t SrcW, bool SrcS>
uint64_t extend(const BitVec<SrcW, SrcS>& val) {
  if constexpr (!SrcS) {
    return static_cast<uint64_t>(val.value());
  } else {
    auto signed_val = static_cast<int64_t>(val.value());
    if constexpr (TargetW >= 64) {
      return static_cast<uint64_t>(signed_val);
    } else {
      return static_cast<uint64_t>(signed_val) & ((uint64_t{1} << TargetW) - 1);
    }
  }
}

}  // namespace internal

// --- Equality (any width/signedness combination) ---

template <size_t W1, bool S1, size_t W2, bool S2>
bool operator==(const BitVec<W1, S1>& lhs, const BitVec<W2, S2>& rhs) {
  constexpr size_t kCommonWidth =
      (S1 == S2) ? std::max(W1, W2) : std::max(W1, W2) + 1;
  uint64_t lhs_ext = internal::extend<kCommonWidth, W1, S1>(lhs);
  uint64_t rhs_ext = internal::extend<kCommonWidth, W2, S2>(rhs);
  return lhs_ext == rhs_ext;
}

template <size_t W1, bool S1, size_t W2, bool S2>
bool operator!=(const BitVec<W1, S1>& lhs, const BitVec<W2, S2>& rhs) {
  return !(lhs == rhs);
}

}  // namespace z3w

#endif  // Z3WIRE_BIT_VEC_H_
