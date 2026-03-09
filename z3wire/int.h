#ifndef Z3WIRE_INT_H_
#define Z3WIRE_INT_H_

#include <algorithm>
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

  // Public for use by internal::extend.
  static constexpr int64_t sign_extend_value(Storage val) {
    return sign_extend(val);
  }

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

  static constexpr int64_t sign_extend(Storage val) {
    if constexpr (W >= 64) {
      return static_cast<int64_t>(val);
    } else {
      uint64_t sign_bit = uint64_t{1} << (W - 1);
      return static_cast<int64_t>((static_cast<uint64_t>(val) ^ sign_bit) -
                                  sign_bit);
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

namespace internal {

template <unsigned TargetW, unsigned SrcW, bool SrcS>
uint64_t extend(const Int<SrcW, SrcS>& val) {
  if constexpr (!SrcS) {
    return val.value();
  } else {
    int64_t signed_val = Int<SrcW, SrcS>::sign_extend_value(val.value());
    if constexpr (TargetW >= 64) {
      return static_cast<uint64_t>(signed_val);
    } else {
      return static_cast<uint64_t>(signed_val) & ((uint64_t{1} << TargetW) - 1);
    }
  }
}

}  // namespace internal

template <unsigned W1, bool S1, unsigned W2, bool S2>
auto operator+(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  constexpr unsigned kResultWidth = std::max(W1, W2) + 1;
  constexpr bool kResultSigned = S1 || S2;
  uint64_t lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  uint64_t rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return Int<kResultWidth, kResultSigned>(lhs_ext + rhs_ext);
}

template <unsigned W1, bool S1, unsigned W2, bool S2>
auto operator-(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  constexpr unsigned kResultWidth = std::max(W1, W2) + 1;
  uint64_t lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  uint64_t rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return Int<kResultWidth, true>(lhs_ext - rhs_ext);
}

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

}  // namespace z3w

#endif  // Z3WIRE_INT_H_
