#ifndef Z3WIRE_BIT_VEC_H_
#define Z3WIRE_BIT_VEC_H_

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>

namespace z3w {

template <size_t W, bool IsSigned>
class BitVec {
  static_assert(W >= 1, "Bit-width must be at least 1.");

  static constexpr size_t kNumBytes = (W + 7) / 8;

  // Smallest unsigned integer type that fits W bits (W <= 64 only).
  using NarrowUnsigned = std::conditional_t<
      (W <= 8), uint8_t,
      std::conditional_t<(W <= 16), uint16_t,
                         std::conditional_t<(W <= 32), uint32_t, uint64_t>>>;

  // Smallest signed integer type that fits W bits (W <= 64 only).
  using NarrowSigned = std::conditional_t<
      (W <= 8), int8_t,
      std::conditional_t<(W <= 16), int16_t,
                         std::conditional_t<(W <= 32), int32_t, int64_t>>>;

 public:
  static constexpr size_t kWidth = W;
  static constexpr bool kIsSigned = IsSigned;

  using ValueType = std::conditional_t<
      (W > 64), std::array<uint8_t, kNumBytes>,
      std::conditional_t<IsSigned, NarrowSigned, NarrowUnsigned>>;

  // Default constructor: zero-initializes.
  constexpr BitVec() : value_{} {}

  // Compile-time range-checked literal.
  template <int64_t Value>
  static constexpr BitVec Literal() {
    if constexpr (W > 64) {
      static_assert(Value >= 0, "Wide unsigned literal must be non-negative.");
      return BitVec(static_cast<uint64_t>(Value));
    } else if constexpr (IsSigned) {
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

  // Runtime checked construction. Returns {result, truncated}.
  template <std::integral T>
  [[nodiscard]] static std::tuple<BitVec, bool> FromValue(T raw) {
    if constexpr (W > 64) {
      if constexpr (std::is_signed_v<T>) {
        if (raw < 0) {
          return {BitVec(static_cast<uint64_t>(raw)), true};
        }
      }
      return {BitVec(static_cast<uint64_t>(raw)), false};
    } else if constexpr (IsSigned) {
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
      bool truncated = (val != result.value_);
      return {result, truncated};
    }
  }

  // Runtime checked construction from byte array (W > 64 only).
  // Returns {result, truncated} where truncated indicates unused high bits
  // were non-zero.
  template <size_t Dummy = W>
    requires(Dummy > 64 && Dummy == W)
  [[nodiscard]] static std::tuple<BitVec, bool> FromValue(
      const std::array<uint8_t, kNumBytes>& bytes) {
    BitVec result;
    result.value_ = bytes;
    // Mask unused bits in the high byte.
    constexpr size_t used_bits = W % 8;
    if constexpr (used_bits != 0) {
      constexpr uint8_t mask = (uint8_t{1} << used_bits) - 1;
      bool truncated = (bytes[kNumBytes - 1] & ~mask) != 0;
      result.value_[kNumBytes - 1] &= mask;
      return {result, truncated};
    } else {
      return {result, false};
    }
  }

  // Access the stored value.
  [[nodiscard]] constexpr ValueType value() const { return value_; }

 private:
  constexpr explicit BitVec(uint64_t raw) : value_(make_value(raw)) {}

  static constexpr ValueType make_value(uint64_t val) {
    if constexpr (W > 64) {
      ValueType bytes{};
      for (size_t i = 0; i < std::min(size_t{8}, kNumBytes); ++i) {
        bytes[i] = static_cast<uint8_t>(val >> (i * 8));
      }
      return bytes;
    } else {
      return truncate(val);
    }
  }

  static constexpr ValueType truncate(uint64_t val)
    requires(W <= 64)
  {
    if constexpr (W == 64) {
      return static_cast<ValueType>(val);
    } else if constexpr (IsSigned) {
      // Mask to W bits, then sign-extend to native width.
      uint64_t masked = val & ((uint64_t{1} << W) - 1);
      uint64_t sign_bit = uint64_t{1} << (W - 1);
      int64_t sign_extended =
          static_cast<int64_t>((masked ^ sign_bit) - sign_bit);
      return static_cast<ValueType>(sign_extended);
    } else {
      return static_cast<ValueType>(val & ((uint64_t{1} << W) - 1));
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

  ValueType value_;
};

template <size_t W>
using UInt = BitVec<W, false>;

template <size_t W>
using SInt = BitVec<W, true>;

// --- Equality (same type: same width and signedness) ---

template <size_t W, bool S>
bool operator==(const BitVec<W, S>& lhs, const BitVec<W, S>& rhs) {
  return lhs.value() == rhs.value();
}

}  // namespace z3w

#endif  // Z3WIRE_BIT_VEC_H_
