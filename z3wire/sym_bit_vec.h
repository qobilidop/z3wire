#ifndef Z3WIRE_SYM_BIT_VEC_H_
#define Z3WIRE_SYM_BIT_VEC_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include <z3++.h>

#include "z3wire/bit_vec.h"
#include "z3wire/sym_bool.h"

namespace z3w {

template <size_t Width, bool IsSigned>
class SymBitVec {
  static_assert(Width > 0, "Width must be at least 1.");

 public:
  static constexpr size_t kWidth = Width;
  static constexpr bool kIsSigned = IsSigned;

  // Default constructor: creates an uninitialized SymBitVec.
  SymBitVec() = default;

  // Create a symbolic bit-vector variable.
  explicit SymBitVec(z3::context& ctx, const std::string& name)
      : expr_(ctx.bv_const(name.c_str(), Width)) {}

  // Create a SymBitVec from a raw z3::expr. Caller must ensure correct sort.
  explicit SymBitVec(z3::expr expr) : expr_(std::move(expr)) {}

  // Compile-time range-checked literal.
  template <uint64_t Value>
  static SymBitVec Literal(z3::context& ctx) {
    static_assert(Width >= 64 || Value < (uint64_t{1} << Width),
                  "Literal value does not fit in the specified bit-width.");
    return SymBitVec(ctx.bv_val(Value, Width));
  }

  [[nodiscard]] const z3::expr& expr() const { return *expr_; }

  // --- Bitwise operators (strict: same width and signedness) ---

  friend SymBitVec operator&(const SymBitVec& lhs, const SymBitVec& rhs) {
    return SymBitVec(*lhs.expr_ & *rhs.expr_);
  }

  friend SymBitVec operator|(const SymBitVec& lhs, const SymBitVec& rhs) {
    return SymBitVec(*lhs.expr_ | *rhs.expr_);
  }

  friend SymBitVec operator^(const SymBitVec& lhs, const SymBitVec& rhs) {
    return SymBitVec(*lhs.expr_ ^ *rhs.expr_);
  }

  SymBitVec operator~() const { return SymBitVec(~*expr_); }

 private:
  std::optional<z3::expr> expr_;
};

template <size_t W>
using SymUInt = SymBitVec<W, false>;

template <size_t W>
using SymSInt = SymBitVec<W, true>;

// Type trait: is this a symbolic SymBitVec type?
template <typename T>
struct is_symbolic : std::false_type {};

template <size_t W, bool S>
struct is_symbolic<SymBitVec<W, S>> : std::true_type {};

template <>
struct is_symbolic<SymBool> : std::true_type {};

template <typename T>
inline constexpr bool is_symbolic_v = is_symbolic<T>::value;

// Convert a concrete BitVec to a symbolic SymBitVec.
template <size_t W, bool S>
SymBitVec<W, S> to_symbolic(const BitVec<W, S>& val, z3::context& ctx) {
  return SymBitVec<W, S>(ctx.bv_val(static_cast<uint64_t>(val.bits()), W));
}

// Convert a symbolic SymBitVec to a concrete BitVec using a solved model.
template <size_t W, bool S>
BitVec<W, S> to_concrete(const SymBitVec<W, S>& symbolic,
                         const z3::model& model) {
  static_assert(W <= 64, "to_concrete requires width <= 64.");
  if constexpr (S) {
    return std::get<0>(BitVec<W, S>::checked(
        model.eval(symbolic.expr(), true).get_numeral_int64()));
  } else {
    return std::get<0>(BitVec<W, S>::checked(
        model.eval(symbolic.expr(), true).get_numeral_uint64()));
  }
}

// --- Mixed operand support ---
// Promotes concrete operands to symbolic, delegating to the pure-symbolic
// operators. Requires exactly one of the two operands to be concrete.

namespace internal {

// Get the z3::context from whichever operand is symbolic.
template <typename L, typename R>
z3::context& get_ctx(const L& lhs, const R& rhs) {
  if constexpr (is_symbolic_v<L>) {
    return lhs.expr().ctx();
  } else {
    return rhs.expr().ctx();
  }
}

// Promote a value to symbolic if it isn't already.
template <typename T>
decltype(auto) promote(const T& val, z3::context& ctx) {
  if constexpr (is_symbolic_v<T>) {
    return val;
  } else {
    return to_symbolic(val, ctx);
  }
}

}  // namespace internal

// True when exactly one operand is concrete and the other is symbolic.
template <typename L, typename R>
concept mixed_operands = (is_symbolic_v<L> && is_concrete_v<R>) ||
                         (is_concrete_v<L> && is_symbolic_v<R>);

// --- Bit-growth arithmetic ---
// Result width = max(W1, W2) + 1. Operands are extended to the result width
// before the operation.

namespace internal {

template <size_t TargetWidth, size_t SrcWidth, bool SrcSigned>
z3::expr extend(const SymBitVec<SrcWidth, SrcSigned>& val) {
  static_assert(TargetWidth >= SrcWidth,
                "extend: target width must be >= source width.");
  if constexpr (TargetWidth == SrcWidth) {
    return val.expr();
  } else if constexpr (SrcSigned) {
    return z3::sext(val.expr(), TargetWidth - SrcWidth);
  } else {
    return z3::zext(val.expr(), TargetWidth - SrcWidth);
  }
}

}  // namespace internal

// Addition: result width = max(W1, W2) + 1.
// Result is signed if either operand is signed.
template <size_t W1, bool S1, size_t W2, bool S2>
auto operator+(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  constexpr size_t kResultWidth = std::max(W1, W2) + 1;
  constexpr bool kResultSigned = S1 || S2;
  auto lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return SymBitVec<kResultWidth, kResultSigned>(lhs_ext + rhs_ext);
}

// Subtraction: result width = max(W1, W2) + 1.
// Result is always signed (subtraction can produce negative results).
template <size_t W1, bool S1, size_t W2, bool S2>
auto operator-(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  constexpr size_t kResultWidth = std::max(W1, W2) + 1;
  auto lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return SymBitVec<kResultWidth, true>(lhs_ext - rhs_ext);
}

// --- Unary negate (bit-growth) ---
// Result width = W + 1, always signed. Consistent with binary subtraction.

template <size_t W, bool S>
SymBitVec<W + 1, true> operator-(const SymBitVec<W, S>& val) {
  auto ext = internal::extend<W + 1, W, S>(val);
  return SymBitVec<W + 1, true>(-ext);
}

// --- Equality (any width/signedness combination) ---

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator==(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  constexpr size_t kCommonWidth =
      (S1 == S2) ? std::max(W1, W2) : std::max(W1, W2) + 1;
  auto lhs_ext = internal::extend<kCommonWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kCommonWidth, W2, S2>(rhs);
  return SymBool(lhs_ext == rhs_ext);
}

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator!=(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  constexpr size_t kCommonWidth =
      (S1 == S2) ? std::max(W1, W2) : std::max(W1, W2) + 1;
  auto lhs_ext = internal::extend<kCommonWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kCommonWidth, W2, S2>(rhs);
  return SymBool(lhs_ext != rhs_ext);
}

// --- Ordered comparison (relaxed: any width/signedness combination) ---

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator<(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  constexpr bool kSigned = S1 || S2;
  constexpr size_t kCommonWidth =
      (S1 == S2) ? std::max(W1, W2) : std::max(W1, W2) + 1;
  auto lhs_ext = internal::extend<kCommonWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kCommonWidth, W2, S2>(rhs);
  if constexpr (kSigned) {
    return SymBool(z3::slt(lhs_ext, rhs_ext));
  } else {
    return SymBool(z3::ult(lhs_ext, rhs_ext));
  }
}

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator<=(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  return !(rhs < lhs);
}

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator>(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  return rhs < lhs;
}

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator>=(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  return !(lhs < rhs);
}

// --- Mixed concrete + symbolic operators ---
// A single template per operator handles both sym+conc and conc+sym.

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator+(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) + internal::promote(rhs, ctx);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator-(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) - internal::promote(rhs, ctx);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator&(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) & internal::promote(rhs, ctx);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator|(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) | internal::promote(rhs, ctx);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator^(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) ^ internal::promote(rhs, ctx);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator==(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) == internal::promote(rhs, ctx);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator!=(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) != internal::promote(rhs, ctx);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator<(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) < internal::promote(rhs, ctx);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator<=(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) <= internal::promote(rhs, ctx);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator>(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) > internal::promote(rhs, ctx);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto operator>=(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) >= internal::promote(rhs, ctx);
}

// --- Mixed ite ---

// SymBool condition with concrete values.
template <size_t W, bool S>
SymBitVec<W, S> ite(const SymBool& cond, const BitVec<W, S>& true_val,
                    const BitVec<W, S>& false_val) {
  auto& ctx = cond.expr().ctx();
  return ite(cond, to_symbolic(true_val, ctx), to_symbolic(false_val, ctx));
}

// SymBool condition with mixed values.
template <size_t W, bool S>
SymBitVec<W, S> ite(const SymBool& cond, const SymBitVec<W, S>& true_val,
                    const BitVec<W, S>& false_val) {
  auto& ctx = cond.expr().ctx();
  return ite(cond, true_val, to_symbolic(false_val, ctx));
}

template <size_t W, bool S>
SymBitVec<W, S> ite(const SymBool& cond, const BitVec<W, S>& true_val,
                    const SymBitVec<W, S>& false_val) {
  auto& ctx = cond.expr().ctx();
  return ite(cond, to_symbolic(true_val, ctx), false_val);
}

// bool condition with symbolic values.
template <size_t W, bool S>
SymBitVec<W, S> ite(bool cond, const SymBitVec<W, S>& true_val,
                    const SymBitVec<W, S>& false_val) {
  return cond ? true_val : false_val;
}

// --- Three-tier casting API ---

// unsafe_cast<T>(val): raw hardware cast (truncation, extension, or bitcast).
template <typename Target, size_t SrcW, bool SrcS>
Target unsafe_cast(const SymBitVec<SrcW, SrcS>& val) {
  constexpr size_t kTgtW = Target::kWidth;
  if constexpr (kTgtW == SrcW) {
    // Same width: zero-overhead type reinterpretation.
    return Target(val.expr());
  } else if constexpr (kTgtW < SrcW) {
    // Truncation.
    return Target(val.expr().extract(kTgtW - 1, 0));
  } else {
    // Extension: sign-extend if source is signed, zero-extend otherwise.
    if constexpr (SrcS) {
      return Target(z3::sext(val.expr(), kTgtW - SrcW));
    } else {
      return Target(z3::zext(val.expr(), kTgtW - SrcW));
    }
  }
}

// safe_cast<T>(val): only compiles if the cast is guaranteed lossless.
template <typename Target, size_t SrcW, bool SrcS>
Target safe_cast(const SymBitVec<SrcW, SrcS>& val) {
  constexpr size_t kTgtW = Target::kWidth;
  constexpr bool kTgtS = Target::kIsSigned;

  // Signed-to-unsigned is always forbidden.
  static_assert(!SrcS || kTgtS,
                "safe_cast from signed to unsigned is always forbidden.");

  if constexpr (!SrcS && !kTgtS) {
    // SymUInt -> SymUInt: target must be at least as wide.
    static_assert(kTgtW >= SrcW,
                  "safe_cast: target width too small for lossless conversion.");
  } else if constexpr (SrcS && kTgtS) {
    // SymSInt -> SymSInt: target must be at least as wide.
    static_assert(kTgtW >= SrcW,
                  "safe_cast: target width too small for lossless conversion.");
  } else if constexpr (!SrcS && kTgtS) {
    // SymUInt -> SymSInt: need 1 extra bit for the sign.
    static_assert(kTgtW > SrcW,
                  "safe_cast: unsigned-to-signed needs target width > source "
                  "width.");
  }

  return unsafe_cast<Target>(val);
}

// checked_cast<T>(val): returns {result, value_preserved}.
template <typename Target, size_t SrcW, bool SrcS>
[[nodiscard]] std::tuple<Target, SymBool> checked_cast(
    const SymBitVec<SrcW, SrcS>& val) {
  auto result = unsafe_cast<Target>(val);
  // Round-trip: cast back to source type and check equality.
  auto roundtrip = unsafe_cast<SymBitVec<SrcW, SrcS>>(result);
  SymBool value_preserved = (roundtrip == val);
  return {result, value_preserved};
}

// --- Same-width reinterpretation ---

// as_unsigned: reinterpret as unsigned, same width.
template <size_t W, bool S>
SymUInt<W> as_unsigned(const SymBitVec<W, S>& val) {
  return unsafe_cast<SymUInt<W>>(val);
}

// as_signed: reinterpret as signed, same width.
template <size_t W, bool S>
SymSInt<W> as_signed(const SymBitVec<W, S>& val) {
  return unsafe_cast<SymSInt<W>>(val);
}

// --- SymBool / SymUInt<1> conversion ---

inline SymUInt<1> as_uint1(const SymBool& b) {
  return SymUInt<1>(z3::ite(b.expr(), b.expr().ctx().bv_val(1, 1),
                            b.expr().ctx().bv_val(0, 1)));
}

inline SymBool as_bool(const SymUInt<1>& v) {
  return SymBool(v.expr() == v.expr().ctx().bv_val(1, 1));
}

// --- Bit slicing (extract) ---

// Static extract: extract<High, Low>(val) -> SymUInt<High - Low + 1>.
template <size_t High, size_t Low, size_t W, bool S>
SymUInt<High - Low + 1> extract(const SymBitVec<W, S>& val) {
  static_assert(High >= Low, "extract: High must be >= Low.");
  static_assert(High < W, "extract: High must be < input width.");
  return SymUInt<High - Low + 1>(val.expr().extract(High, Low));
}

// Symbolic-offset extract: shift right by offset, then static extract.
template <size_t TargetWidth, size_t W, bool S, size_t IdxW>
SymUInt<TargetWidth> extract(const SymBitVec<W, S>& val,
                             const SymUInt<IdxW>& start_idx) {
  static_assert(TargetWidth > 0, "extract: TargetWidth must be > 0.");
  static_assert(TargetWidth <= W,
                "extract: TargetWidth must be <= input width.");
  // Extend index to match input width for the shift.
  auto idx_ext = z3::zext(start_idx.expr(), W - IdxW);
  auto shifted = z3::lshr(val.expr(), idx_ext);
  return SymUInt<TargetWidth>(shifted.extract(TargetWidth - 1, 0));
}

// Single-bit extraction: bit<N>(val) -> SymUInt<1>.
template <size_t N, size_t W, bool S>
SymUInt<1> bit(const SymBitVec<W, S>& val) {
  return extract<N, N>(val);
}

// --- Concatenation ---

// concat(a, b): result width = W1 + W2, always SymUInt.
template <size_t W1, bool S1, size_t W2, bool S2>
SymUInt<W1 + W2> concat(const SymBitVec<W1, S1>& high,
                        const SymBitVec<W2, S2>& low) {
  return SymUInt<W1 + W2>(z3::concat(high.expr(), low.expr()));
}

// Variadic concat.
template <size_t W1, bool S1, size_t W2, bool S2, typename... Rest>
auto concat(const SymBitVec<W1, S1>& high, const SymBitVec<W2, S2>& next,
            const Rest&... rest) {
  return concat(concat(high, next), rest...);
}

// --- Left shift (lossless, auto-widening) ---

// Constant shift: result width = W + N.
template <size_t N, size_t W, bool S>
SymUInt<W + N> shl(const SymBitVec<W, S>& val) {
  auto widened = unsafe_cast<SymUInt<W + N>>(val);
  auto amount = SymUInt<W + N>::template Literal<N>(val.expr().ctx());
  return SymUInt<W + N>(z3::shl(widened.expr(), amount.expr()));
}

// Symbolic shift: result width = W + 2^K - 1.
template <size_t W, bool S, size_t K>
auto shl(const SymBitVec<W, S>& val, const BitVec<K, false>& amount) {
  return shl(val, to_symbolic(amount, val.expr().ctx()));
}

template <size_t W, bool S, size_t K>
auto shl(const SymBitVec<W, S>& val, const SymUInt<K>& amount) {
  constexpr size_t kMaxShift = (size_t{1} << K) - 1;
  constexpr size_t kResultWidth = W + kMaxShift;
  auto widened = unsafe_cast<SymUInt<kResultWidth>>(val);
  auto amt_ext = unsafe_cast<SymUInt<kResultWidth>>(amount);
  return SymUInt<kResultWidth>(z3::shl(widened.expr(), amt_ext.expr()));
}

// --- Right shift (arithmetic) ---

// Constant shift: result width = W (same as input).
template <size_t N, size_t W, bool S>
SymBitVec<W, S> shr(const SymBitVec<W, S>& val) {
  auto amount = SymUInt<W>::template Literal<N>(val.expr().ctx());
  if constexpr (S) {
    return SymBitVec<W, S>(z3::ashr(val.expr(), amount.expr()));
  } else {
    return SymBitVec<W, S>(z3::lshr(val.expr(), amount.expr()));
  }
}

// Symbolic shift: amount width may differ from value width.
template <size_t W, bool S, size_t K>
SymBitVec<W, S> shr(const SymBitVec<W, S>& val,
                    const BitVec<K, false>& amount) {
  return shr(val, to_symbolic(amount, val.expr().ctx()));
}

template <size_t W, bool S, size_t K>
SymBitVec<W, S> shr(const SymBitVec<W, S>& val, const SymUInt<K>& amount) {
  auto amt_ext = unsafe_cast<SymUInt<W>>(amount);
  if constexpr (S) {
    return SymBitVec<W, S>(z3::ashr(val.expr(), amt_ext.expr()));
  } else {
    return SymBitVec<W, S>(z3::lshr(val.expr(), amt_ext.expr()));
  }
}

// --- Conditional selection (ite) ---

template <size_t W, bool S>
SymBitVec<W, S> ite(const SymBool& cond, const SymBitVec<W, S>& true_val,
                    const SymBitVec<W, S>& false_val) {
  return SymBitVec<W, S>(
      z3::ite(cond.expr(), true_val.expr(), false_val.expr()));
}

}  // namespace z3w

#endif  // Z3WIRE_SYM_BIT_VEC_H_
