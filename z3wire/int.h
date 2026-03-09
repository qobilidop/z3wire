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
