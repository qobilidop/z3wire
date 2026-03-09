#ifndef Z3WIRE_BITVEC_H_
#define Z3WIRE_BITVEC_H_

#include <z3++.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include "z3wire/bool.h"
#include "z3wire/int.h"

namespace z3w {

template <size_t Width, bool IsSigned>
class BitVec {
  static_assert(Width > 0, "Bit-vector width must be at least 1.");

 public:
  static constexpr size_t kWidth = Width;
  static constexpr bool kIsSigned = IsSigned;

  // Create a symbolic bit-vector variable.
  BitVec(z3::context& ctx, const std::string& name)
      : expr_(ctx.bv_const(name.c_str(), Width)) {}

  // Create a BitVec from a raw z3::expr. Caller must ensure correct sort.
  explicit BitVec(z3::expr expr) : expr_(std::move(expr)) {}

  // Compile-time range-checked literal.
  template <uint64_t Value>
  static BitVec Literal(z3::context& ctx) {
    if constexpr (IsSigned) {
      static_assert(Width >= 64 || Value < (uint64_t{1} << Width),
                    "Literal value does not fit in the specified bit-width.");
    } else {
      static_assert(Width >= 64 || Value < (uint64_t{1} << Width),
                    "Literal value does not fit in the specified bit-width.");
    }
    return BitVec(ctx.bv_val(Value, Width));
  }

  [[nodiscard]] const z3::expr& raw() const { return expr_; }

  // --- Bitwise operators (strict: same width and signedness) ---

  friend BitVec operator&(const BitVec& lhs, const BitVec& rhs) {
    return BitVec(lhs.expr_ & rhs.expr_);
  }

  friend BitVec operator|(const BitVec& lhs, const BitVec& rhs) {
    return BitVec(lhs.expr_ | rhs.expr_);
  }

  friend BitVec operator^(const BitVec& lhs, const BitVec& rhs) {
    return BitVec(lhs.expr_ ^ rhs.expr_);
  }

  BitVec operator~() const { return BitVec(~expr_); }

  // --- Equality (strict: same width and signedness) ---

  friend Bool operator==(const BitVec& lhs, const BitVec& rhs) {
    return Bool(lhs.expr_ == rhs.expr_);
  }

  friend Bool operator!=(const BitVec& lhs, const BitVec& rhs) {
    return Bool(lhs.expr_ != rhs.expr_);
  }

  // --- Ordered comparison (strict, signedness-aware) ---

  friend Bool operator<(const BitVec& lhs, const BitVec& rhs) {
    if constexpr (IsSigned) {
      return Bool(z3::slt(lhs.expr_, rhs.expr_));
    } else {
      return Bool(z3::ult(lhs.expr_, rhs.expr_));
    }
  }

  friend Bool operator<=(const BitVec& lhs, const BitVec& rhs) {
    if constexpr (IsSigned) {
      return Bool(z3::sle(lhs.expr_, rhs.expr_));
    } else {
      return Bool(z3::ule(lhs.expr_, rhs.expr_));
    }
  }

  friend Bool operator>(const BitVec& lhs, const BitVec& rhs) {
    if constexpr (IsSigned) {
      return Bool(z3::sgt(lhs.expr_, rhs.expr_));
    } else {
      return Bool(z3::ugt(lhs.expr_, rhs.expr_));
    }
  }

  friend Bool operator>=(const BitVec& lhs, const BitVec& rhs) {
    if constexpr (IsSigned) {
      return Bool(z3::sge(lhs.expr_, rhs.expr_));
    } else {
      return Bool(z3::uge(lhs.expr_, rhs.expr_));
    }
  }

  // --- Hardware shifts (strict: same width and signedness) ---

  friend BitVec operator<<(const BitVec& lhs, const BitVec& rhs) {
    return BitVec(z3::shl(lhs.expr_, rhs.expr_));
  }

  friend BitVec operator>>(const BitVec& lhs, const BitVec& rhs) {
    if constexpr (IsSigned) {
      return BitVec(z3::ashr(lhs.expr_, rhs.expr_));
    } else {
      return BitVec(z3::lshr(lhs.expr_, rhs.expr_));
    }
  }

 private:
  z3::expr expr_;
};

template <size_t W>
using Ubv = BitVec<W, false>;

template <size_t W>
using Sbv = BitVec<W, true>;

// Type trait: is this a symbolic BitVec type?
template <typename T>
struct is_symbolic : std::false_type {};

template <size_t W, bool S>
struct is_symbolic<BitVec<W, S>> : std::true_type {};

template <>
struct is_symbolic<Bool> : std::true_type {};

template <typename T>
inline constexpr bool is_symbolic_v = is_symbolic<T>::value;

// Convert a concrete Int to a symbolic BitVec.
template <unsigned W, bool S>
BitVec<W, S> to_symbolic(const Int<W, S>& val, z3::context& ctx) {
  return BitVec<W, S>(ctx.bv_val(static_cast<uint64_t>(val.value()), W));
}

// --- Bit-growth arithmetic ---
// Result width = max(W1, W2) + 1. Operands are extended to the result width
// before the operation.

namespace internal {

template <size_t TargetWidth, size_t SrcWidth, bool SrcSigned>
z3::expr extend(const BitVec<SrcWidth, SrcSigned>& val) {
  static_assert(TargetWidth >= SrcWidth);
  if constexpr (TargetWidth == SrcWidth) {
    return val.raw();
  } else if constexpr (SrcSigned) {
    return z3::sext(val.raw(), TargetWidth - SrcWidth);
  } else {
    return z3::zext(val.raw(), TargetWidth - SrcWidth);
  }
}

}  // namespace internal

// Addition: result width = max(W1, W2) + 1.
// Result is signed if either operand is signed.
template <size_t W1, bool S1, size_t W2, bool S2>
auto operator+(const BitVec<W1, S1>& lhs, const BitVec<W2, S2>& rhs) {
  constexpr size_t kResultWidth = std::max(W1, W2) + 1;
  constexpr bool kResultSigned = S1 || S2;
  auto lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return BitVec<kResultWidth, kResultSigned>(lhs_ext + rhs_ext);
}

// Subtraction: result width = max(W1, W2) + 1.
// Result is always signed (subtraction can produce negative results).
template <size_t W1, bool S1, size_t W2, bool S2>
auto operator-(const BitVec<W1, S1>& lhs, const BitVec<W2, S2>& rhs) {
  constexpr size_t kResultWidth = std::max(W1, W2) + 1;
  auto lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return BitVec<kResultWidth, true>(lhs_ext - rhs_ext);
}

// --- Mixed concrete + symbolic arithmetic ---

// Addition: symbolic + concrete.
template <size_t W1, bool S1, unsigned W2, bool S2>
auto operator+(const BitVec<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  auto& ctx = lhs.raw().ctx();
  return lhs + to_symbolic(rhs, ctx);
}

// Addition: concrete + symbolic.
template <unsigned W1, bool S1, size_t W2, bool S2>
auto operator+(const Int<W1, S1>& lhs, const BitVec<W2, S2>& rhs) {
  auto& ctx = rhs.raw().ctx();
  return to_symbolic(lhs, ctx) + rhs;
}

// Subtraction: symbolic - concrete.
template <size_t W1, bool S1, unsigned W2, bool S2>
auto operator-(const BitVec<W1, S1>& lhs, const Int<W2, S2>& rhs) {
  auto& ctx = lhs.raw().ctx();
  return lhs - to_symbolic(rhs, ctx);
}

// Subtraction: concrete - symbolic.
template <unsigned W1, bool S1, size_t W2, bool S2>
auto operator-(const Int<W1, S1>& lhs, const BitVec<W2, S2>& rhs) {
  auto& ctx = rhs.raw().ctx();
  return to_symbolic(lhs, ctx) - rhs;
}

// --- Mixed bitwise operators ---

// NOLINTBEGIN(modernize-use-constraints)
template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W1, S> operator&(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs & to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W2, S> operator&(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) & rhs;
}

template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W1, S> operator|(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs | to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W2, S> operator|(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) | rhs;
}

template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W1, S> operator^(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs ^ to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W2, S> operator^(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) ^ rhs;
}

// --- Mixed shift operators ---

template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W1, S> operator<<(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs << to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W2, S> operator<<(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) << rhs;
}

template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W1, S> operator>>(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs >> to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W2, S> operator>>(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) >> rhs;
}

// --- Mixed comparison operators ---

template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator==(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs == to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator==(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) == rhs;
}

template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator!=(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs != to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator!=(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) != rhs;
}

template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator<(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs < to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator<(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) < rhs;
}

template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator<=(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs <= to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator<=(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) <= rhs;
}

template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator>(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs > to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator>(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
  return to_symbolic(lhs, rhs.raw().ctx()) > rhs;
}

template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator>=(const BitVec<W1, S>& lhs, const Int<W2, S>& rhs) {
  return lhs >= to_symbolic(rhs, lhs.raw().ctx());
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
Bool operator>=(const Int<W1, S>& lhs, const BitVec<W2, S>& rhs) {
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
template <size_t W1, bool S, unsigned W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W1, S> ite(const Bool& cond, const BitVec<W1, S>& true_val,
                  const Int<W2, S>& false_val) {
  auto& ctx = cond.raw().ctx();
  return ite(cond, true_val, to_symbolic(false_val, ctx));
}

template <unsigned W1, bool S, size_t W2, std::enable_if_t<W1 == W2, int> = 0>
BitVec<W2, S> ite(const Bool& cond, const Int<W1, S>& true_val,
                  const BitVec<W2, S>& false_val) {
  auto& ctx = cond.raw().ctx();
  return ite(cond, to_symbolic(true_val, ctx), false_val);
}
// NOLINTEND(modernize-use-constraints)

// bool condition with symbolic values.
template <size_t W, bool S>
BitVec<W, S> ite(bool cond, const BitVec<W, S>& true_val,
                 const BitVec<W, S>& false_val) {
  return cond ? true_val : false_val;
}

// --- Three-tier casting API ---

// cast<T>(val): raw hardware cast (truncation, extension, or bitcast).
template <typename Target, size_t SrcW, bool SrcS>
Target cast(const BitVec<SrcW, SrcS>& val) {
  constexpr size_t kTgtW = Target::kWidth;
  if constexpr (kTgtW == SrcW) {
    // Same width: zero-overhead type reinterpretation.
    return Target(val.raw());
  } else if constexpr (kTgtW < SrcW) {
    // Truncation.
    return Target(val.raw().extract(kTgtW - 1, 0));
  } else {
    // Extension: sign-extend if source is signed, zero-extend otherwise.
    if constexpr (SrcS) {
      return Target(z3::sext(val.raw(), kTgtW - SrcW));
    } else {
      return Target(z3::zext(val.raw(), kTgtW - SrcW));
    }
  }
}

// safe_cast<T>(val): only compiles if the cast is guaranteed lossless.
template <typename Target, size_t SrcW, bool SrcS>
Target safe_cast(const BitVec<SrcW, SrcS>& val) {
  constexpr size_t kTgtW = Target::kWidth;
  constexpr bool kTgtS = Target::kIsSigned;

  // Signed-to-unsigned is always forbidden.
  static_assert(!SrcS || kTgtS,
                "safe_cast from signed to unsigned is always forbidden.");

  if constexpr (!SrcS && !kTgtS) {
    // Ubv -> Ubv: target must be at least as wide.
    static_assert(kTgtW >= SrcW,
                  "safe_cast: target width too small for lossless conversion.");
  } else if constexpr (SrcS && kTgtS) {
    // Sbv -> Sbv: target must be at least as wide.
    static_assert(kTgtW >= SrcW,
                  "safe_cast: target width too small for lossless conversion.");
  } else if constexpr (!SrcS && kTgtS) {
    // Ubv -> Sbv: need 1 extra bit for the sign.
    static_assert(kTgtW > SrcW,
                  "safe_cast: unsigned-to-signed needs target width > source "
                  "width.");
  }

  return cast<Target>(val);
}

// checked_cast<T>(val): returns {result, overflow_flag}.
template <typename Target, size_t SrcW, bool SrcS>
std::pair<Target, Bool> checked_cast(const BitVec<SrcW, SrcS>& val) {
  auto result = cast<Target>(val);
  // Round-trip: cast back to source type and check equality.
  auto roundtrip = cast<BitVec<SrcW, SrcS>>(result);
  Bool overflowed = (roundtrip != val);
  return {result, overflowed};
}

// --- Bool / Ubv<1> conversion ---

inline Ubv<1> to_ubv1(const Bool& b) {
  return Ubv<1>(
      z3::ite(b.raw(), b.raw().ctx().bv_val(1, 1), b.raw().ctx().bv_val(0, 1)));
}

inline Bool to_bool(const Ubv<1>& v) {
  return Bool(v.raw() == v.raw().ctx().bv_val(1, 1));
}

// --- Bit slicing (extract) ---

// Static extract: extract<High, Low>(val) -> Ubv<High - Low + 1>.
template <size_t High, size_t Low, size_t W, bool S>
Ubv<High - Low + 1> extract(const BitVec<W, S>& val) {
  static_assert(High >= Low, "extract: High must be >= Low.");
  static_assert(High < W, "extract: High must be < input width.");
  return Ubv<High - Low + 1>(val.raw().extract(High, Low));
}

// Symbolic-offset extract: shift right by offset, then static extract.
template <size_t TargetWidth, size_t W, bool S, size_t IdxW>
Ubv<TargetWidth> extract(const BitVec<W, S>& val, const Ubv<IdxW>& start_idx) {
  static_assert(TargetWidth > 0, "extract: TargetWidth must be > 0.");
  static_assert(TargetWidth <= W,
                "extract: TargetWidth must be <= input width.");
  // Extend index to match input width for the shift.
  auto idx_ext = z3::zext(start_idx.raw(), W - IdxW);
  auto shifted = z3::lshr(val.raw(), idx_ext);
  return Ubv<TargetWidth>(shifted.extract(TargetWidth - 1, 0));
}

// --- Concatenation ---

// concat(a, b): result width = W1 + W2, always Ubv.
template <size_t W1, bool S1, size_t W2, bool S2>
Ubv<W1 + W2> concat(const BitVec<W1, S1>& high, const BitVec<W2, S2>& low) {
  return Ubv<W1 + W2>(z3::concat(high.raw(), low.raw()));
}

// Variadic concat.
template <size_t W1, bool S1, size_t W2, bool S2, typename... Rest>
auto concat(const BitVec<W1, S1>& high, const BitVec<W2, S2>& next,
            const Rest&... rest) {
  return concat(concat(high, next), rest...);
}

// --- Checked shifts ---

// checked_shl: returns {shifted, lost} where lost is true if any bits were
// shifted out.
template <size_t W, bool S>
std::pair<BitVec<W, S>, Bool> checked_shl(const BitVec<W, S>& val,
                                          const BitVec<W, S>& amount) {
  auto shifted = val << amount;
  // Check by shifting back and comparing.
  auto restored = shifted >> amount;
  Bool lost = (restored != val);
  return {shifted, lost};
}

// checked_shr: returns {shifted, lost} where lost is true if any bits were
// shifted out.
template <size_t W, bool S>
std::pair<BitVec<W, S>, Bool> checked_shr(const BitVec<W, S>& val,
                                          const BitVec<W, S>& amount) {
  auto shifted = val >> amount;
  auto restored = shifted << amount;
  Bool lost = (restored != val);
  return {shifted, lost};
}

// --- Lossless left shift ---

// Constant shift: result width = W + N.
template <size_t N, size_t W, bool S>
Ubv<W + N> lossless_shl(const BitVec<W, S>& val) {
  auto widened = cast<Ubv<W + N>>(val);
  auto amount = Ubv<W + N>::template Literal<N>(val.raw().ctx());
  return Ubv<W + N>(z3::shl(widened.raw(), amount.raw()));
}

// Symbolic shift: result width = W + 2^K - 1.
template <size_t W, bool S, size_t K>
auto lossless_shl(const BitVec<W, S>& val, const Ubv<K>& amount) {
  constexpr size_t kMaxShift = (size_t{1} << K) - 1;
  constexpr size_t kResultWidth = W + kMaxShift;
  auto widened = cast<Ubv<kResultWidth>>(val);
  auto amt_ext = cast<Ubv<kResultWidth>>(amount);
  return Ubv<kResultWidth>(z3::shl(widened.raw(), amt_ext.raw()));
}

// --- Conditional selection (ite) ---

template <size_t W, bool S>
BitVec<W, S> ite(const Bool& cond, const BitVec<W, S>& true_val,
                 const BitVec<W, S>& false_val) {
  return BitVec<W, S>(z3::ite(cond.raw(), true_val.raw(), false_val.raw()));
}

}  // namespace z3w

#endif  // Z3WIRE_BITVEC_H_
