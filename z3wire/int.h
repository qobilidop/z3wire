#ifndef Z3WIRE_INT_H_
#define Z3WIRE_INT_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace z3w {

// Storage: smallest unsigned integer type that fits W bits.
template <size_t W>
using StorageType = std::conditional_t<
    (W <= 8), uint8_t,
    std::conditional_t<(W <= 16), uint16_t,
                       std::conditional_t<(W <= 32), uint32_t, uint64_t>>>;

template <size_t W, bool IsSigned>
class Int {
  static_assert(W >= 1 && W <= 64, "Bit-width must be between 1 and 64.");

 public:
  static constexpr size_t kWidth = W;
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

  // Runtime checked construction. Takes an unsigned raw bit pattern.
  // Note: for signed types, pass the two's complement bit pattern, not a
  // negative integer. E.g., use SInt<8>::checked(0x80) for -128, not
  // SInt<8>::checked(-128).
  static std::pair<Int, bool> checked(uint64_t raw) {
    Int result(raw);
    bool truncated = (raw != result.bits_);
    return {result, truncated};
  }

  // Runtime checked construction from a signed value.
  // Only meaningful for signed types; for unsigned, use checked(uint64_t).
  // NOLINTNEXTLINE(modernize-use-constraints)
  template <bool Signed = IsSigned, typename = std::enable_if_t<Signed>>
  static std::pair<Int, bool> checked(int64_t raw) {
    Int result(static_cast<uint64_t>(raw));
    bool truncated = (raw < min_signed() || raw > max_signed());
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

template <size_t W>
using UInt = Int<W, false>;

template <size_t W>
using SInt = Int<W, true>;

// Type trait: is this a concrete Int type?
template <typename T>
struct is_concrete : std::false_type {};

template <size_t W, bool S>
struct is_concrete<Int<W, S>> : std::true_type {};

template <typename T>
inline constexpr bool is_concrete_v = is_concrete<T>::value;

namespace internal {

template <size_t TargetW, size_t SrcW, bool SrcS>
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

template <size_t W1, bool S1, size_t W2, bool S2>
auto operator+(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  constexpr size_t kResultWidth = std::max(W1, W2) + 1;
  constexpr bool kResultSigned = S1 || S2;
  uint64_t lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  uint64_t rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return Int<kResultWidth, kResultSigned>(lhs_ext + rhs_ext);
}

template <size_t W1, bool S1, size_t W2, bool S2>
auto operator-(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  constexpr size_t kResultWidth = std::max(W1, W2) + 1;
  uint64_t lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  uint64_t rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return Int<kResultWidth, true>(lhs_ext - rhs_ext);
}

// --- Equality (relaxed: any width/signedness combination) ---

template <size_t W1, bool S1, size_t W2, bool S2>
bool operator==(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  constexpr size_t kCommonWidth =
      (S1 == S2) ? std::max(W1, W2) : std::max(W1, W2) + 1;
  uint64_t lhs_ext = internal::extend<kCommonWidth, W1, S1>(lhs);
  uint64_t rhs_ext = internal::extend<kCommonWidth, W2, S2>(rhs);
  return lhs_ext == rhs_ext;
}

template <size_t W1, bool S1, size_t W2, bool S2>
bool operator!=(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  return !(lhs == rhs);
}

// --- Ordered comparison (relaxed: any width/signedness combination) ---

template <size_t W1, bool S1, size_t W2, bool S2>
bool operator<(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  constexpr bool kSigned = S1 || S2;
  constexpr size_t kCommonWidth =
      (S1 == S2) ? std::max(W1, W2) : std::max(W1, W2) + 1;
  uint64_t lhs_ext = internal::extend<kCommonWidth, W1, S1>(lhs);
  uint64_t rhs_ext = internal::extend<kCommonWidth, W2, S2>(rhs);
  if constexpr (kSigned) {
    return Int<kCommonWidth, true>::sign_extend_value(
               static_cast<StorageType<kCommonWidth>>(lhs_ext)) <
           Int<kCommonWidth, true>::sign_extend_value(
               static_cast<StorageType<kCommonWidth>>(rhs_ext));
  } else {
    return lhs_ext < rhs_ext;
  }
}

template <size_t W1, bool S1, size_t W2, bool S2>
bool operator<=(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  return !(rhs < lhs);
}

template <size_t W1, bool S1, size_t W2, bool S2>
bool operator>(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  return rhs < lhs;
}

template <size_t W1, bool S1, size_t W2, bool S2>
bool operator>=(const Int<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  return !(lhs < rhs);
}

// checked_shl: returns {shifted, lost}.
template <size_t W, bool S>
std::pair<Int<W, S>, bool> checked_shl(const Int<W, S>& val,
                                       const Int<W, S>& amount) {
  auto shifted = val << amount;
  auto restored = shifted >> amount;
  bool lost = (restored != val);
  return {shifted, lost};
}

// checked_shr: returns {shifted, lost}.
template <size_t W, bool S>
std::pair<Int<W, S>, bool> checked_shr(const Int<W, S>& val,
                                       const Int<W, S>& amount) {
  auto shifted = val >> amount;
  auto restored = shifted << amount;
  bool lost = (restored != val);
  return {shifted, lost};
}

// Lossless left shift by constant N: result width = W + N.
template <size_t N, size_t W, bool S>
UInt<W + N> lossless_shl(const Int<W, S>& val) {
  return UInt<W + N>(static_cast<uint64_t>(val.value()) << N);
}

// cast<T>(val): raw hardware cast.
template <typename Target, size_t SrcW, bool SrcS>
Target cast(const Int<SrcW, SrcS>& val) {
  constexpr size_t kTgtW = Target::kWidth;
  if constexpr (kTgtW <= SrcW) {
    // Same width or truncation — constructor masks to target width.
    return Target(val.value());
  } else {
    if constexpr (SrcS) {
      int64_t signed_val = Int<SrcW, SrcS>::sign_extend_value(val.value());
      return Target(static_cast<uint64_t>(signed_val));
    } else {
      return Target(val.value());
    }
  }
}

// safe_cast<T>(val): compile-time lossless check.
template <typename Target, size_t SrcW, bool SrcS>
Target safe_cast(const Int<SrcW, SrcS>& val) {
  constexpr size_t kTgtW = Target::kWidth;
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
    static_assert(
        kTgtW > SrcW,
        "safe_cast: unsigned-to-signed needs target width > source width.");
  }

  return cast<Target>(val);
}

// checked_cast<T>(val): returns {result, overflowed}.
template <typename Target, size_t SrcW, bool SrcS>
std::pair<Target, bool> checked_cast(const Int<SrcW, SrcS>& val) {
  auto result = cast<Target>(val);
  auto roundtrip = cast<Int<SrcW, SrcS>>(result);
  bool overflowed = (roundtrip != val);
  return {result, overflowed};
}

// Static extract: extract<High, Low>(val) -> UInt<High - Low + 1>.
template <size_t High, size_t Low, size_t W, bool S>
UInt<High - Low + 1> extract(const Int<W, S>& val) {
  static_assert(High >= Low, "extract: High must be >= Low.");
  static_assert(High < W, "extract: High must be < input width.");
  return UInt<High - Low + 1>(val.value() >> Low);
}

// concat(a, b): result width = W1 + W2, always UInt.
template <size_t W1, bool S1, size_t W2, bool S2>
UInt<W1 + W2> concat(const Int<W1, S1>& high, const Int<W2, S2>& low) {
  uint64_t result = (static_cast<uint64_t>(high.value()) << W2) |
                    static_cast<uint64_t>(low.value());
  return UInt<W1 + W2>(result);
}

// Variadic concat.
template <size_t W1, bool S1, size_t W2, bool S2, typename... Rest>
auto concat(const Int<W1, S1>& high, const Int<W2, S2>& next,
            const Rest&... rest) {
  return concat(concat(high, next), rest...);
}

// Bool / UInt<1> conversion.
inline UInt<1> to_uint1(bool b) { return UInt<1>(b ? 1 : 0); }

inline bool to_bool(const UInt<1>& v) { return v.value() != 0; }

// ite: concrete conditional selection.
template <size_t W, bool S>
Int<W, S> ite(bool cond, const Int<W, S>& true_val,
              const Int<W, S>& false_val) {
  return cond ? true_val : false_val;
}

}  // namespace z3w

#endif  // Z3WIRE_INT_H_
