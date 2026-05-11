#ifndef Z3WIRE_SYM_BIT_VEC_H_
#define Z3WIRE_SYM_BIT_VEC_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

#include <z3++.h>

#include "z3wire/bit_vec.h"
#include "z3wire/check.h"
#include "z3wire/sym_bool.h"
#include "z3wire/type_traits.h"

namespace z3w {

template <size_t W, bool IsSigned>
class SymBitVec {
  static_assert(W > 0, "Width must be at least 1.");

 public:
  static constexpr size_t kWidth = W;
  static constexpr bool kIsSigned = IsSigned;

  // Default constructor: creates an uninitialized SymBitVec.
  SymBitVec() = default;

  // Create a symbolic bit-vector variable.
  explicit SymBitVec(z3::context& ctx, const std::string& name)
      : expr_(ctx.bv_const(name.c_str(), W)) {}

  // Create a SymBitVec from a raw z3::expr. Aborts if the expression does not
  // have the correct bit-vector sort and width. Prefer TryFrom for a
  // non-aborting alternative.
  explicit SymBitVec(z3::expr expr) : expr_(std::move(expr)) {
    Z3W_CHECK(expr_->get_sort().is_bv() && expr_->get_sort().bv_size() == W)
        << "Expected BitVec sort of width " << W << ", got "
        << expr_->get_sort();
  }

  // Create a SymBitVec from a raw z3::expr; aborts via Z3W_CHECK if the
  // expression doesn't have the correct bit-vector sort and width. Use
  // TryFrom for a non-aborting alternative.
  static SymBitVec From(z3::expr expr) { return SymBitVec(std::move(expr)); }

  // Create a SymBitVec from a raw z3::expr with validation. Returns nullopt
  // if the expression doesn't have the correct bit-vector sort and width.
  static std::optional<SymBitVec> TryFrom(z3::expr expr) {
    if (!expr.get_sort().is_bv() || expr.get_sort().bv_size() != W) {
      return std::nullopt;
    }
    return SymBitVec(std::move(expr));
  }

  // Compile-time range-checked literal.
  template <uint64_t Value>
  static SymBitVec Literal(z3::context& ctx) {
    static_assert(W >= 64 || Value < (uint64_t{1} << W),
                  "Literal value does not fit in bit-width.");
    return SymBitVec(ctx.bv_val(Value, W));
  }

  [[nodiscard]] const z3::expr& expr() const {
    Z3W_CHECK(expr_.has_value()) << "SymBitVec used before initialization";
    return *expr_;
  }

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

// Convert a concrete BitVec to a symbolic SymBitVec.
template <size_t W, bool S>
SymBitVec<W, S> to_symbolic(const BitVec<W, S>& val, z3::context& ctx) {
  if constexpr (W > 64) {
    // Build wide Z3 bit-vector by concatenating 8-bit chunks.
    // Each byte becomes a Z3 bv_val(byte, 8), then we concat MSB-first.
    auto bytes = val.ToLeBytes();
    constexpr size_t kNumBytes = (W + 7) / 8;
    constexpr unsigned kTopBits = ((W % 8) == 0) ? 8 : (W % 8);
    // Start with the most significant byte (possibly narrower than 8 bits).
    z3::expr result =
        ctx.bv_val(static_cast<unsigned>(bytes[kNumBytes - 1]), kTopBits);
    // Concat remaining bytes from high to low.
    for (size_t i = kNumBytes - 1; i > 0; --i) {
      result = z3::concat(result,
                          ctx.bv_val(static_cast<unsigned>(bytes[i - 1]), 8));
    }
    return SymBitVec<W, S>(result);
  } else if constexpr (S) {
    return SymBitVec<W, S>(ctx.bv_val(static_cast<int64_t>(val.value()), W));
  } else {
    return SymBitVec<W, S>(ctx.bv_val(static_cast<uint64_t>(val.value()), W));
  }
}

// Convert a symbolic SymBitVec to a concrete BitVec using a solved model.
template <size_t W, bool S>
BitVec<W, S> to_concrete(const SymBitVec<W, S>& symbolic,
                         const z3::model& model) {
  if constexpr (W > 64) {
    // Extract byte-by-byte from the evaluated Z3 expression.
    z3::expr evaluated = model.eval(symbolic.expr(), true);

    constexpr size_t kNumBytes = (W + 7) / 8;
    std::array<uint8_t, kNumBytes> bytes{};
    for (size_t i = 0; i < kNumBytes; ++i) {
      unsigned hi = std::min(static_cast<unsigned>(i * 8 + 7),
                             static_cast<unsigned>(W - 1));
      unsigned lo = static_cast<unsigned>(i * 8);
      bytes[i] = static_cast<uint8_t>(
          evaluated.extract(hi, lo).simplify().get_numeral_uint());
    }

    return BitVec<W, S>::TryFromLeBytes(bytes).value;
  } else {
    // Always extract as uint64 and let TryFrom() handle sign interpretation.
    // get_numeral_int64() throws for W=64 signed values with the MSB set.
    // Discard the truncated flag: Z3 always returns valid W-bit values; for
    // signed types the uint64 representation can exceed the signed range while
    // the bit pattern is still correct.
    return BitVec<W, S>::TryFrom(
               model.eval(symbolic.expr(), true).get_numeral_uint64())
        .value;
  }
}

// --- Internal helpers ---

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

// Extend a symbolic bit-vector to a wider width.
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

// True when exactly one operand is concrete and the other is symbolic.
template <typename L, typename R>
concept mixed_operands = (is_symbolic_v<L> && is_concrete_v<R>) ||
                         (is_concrete_v<L> && is_symbolic_v<R>);

// --- Bit-growth arithmetic ---
// Result width follows CIRCT hwarith typing rules. Operands are extended to the
// result width before the operation.

// Compute result width for bit-growth arithmetic (CIRCT hwarith rules).
// Same signedness: max(W1, W2) + 1.
// Mixed signedness (ui<A> op si<B>): A+2 if A >= B, else B+1.
template <size_t W1, bool S1, size_t W2, bool S2>
constexpr size_t arith_result_width() {
  if constexpr (S1 == S2) {
    return std::max(W1, W2) + 1;
  } else if constexpr (!S1 && S2) {
    // unsigned lhs, signed rhs
    return (W1 >= W2) ? W1 + 2 : W2 + 1;
  } else {
    // signed lhs, unsigned rhs: same as swapped
    return (W2 >= W1) ? W2 + 2 : W1 + 1;
  }
}

// Addition: result is signed if either operand is signed.
template <size_t W1, bool S1, size_t W2, bool S2>
auto operator+(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  constexpr size_t kResultWidth = arith_result_width<W1, S1, W2, S2>();
  constexpr bool kResultSigned = S1 || S2;
  auto lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return SymBitVec<kResultWidth, kResultSigned>(lhs_ext + rhs_ext);
}

// Subtraction: result is always signed (subtraction can produce negative
// results).
template <size_t W1, bool S1, size_t W2, bool S2>
auto operator-(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  constexpr size_t kResultWidth = arith_result_width<W1, S1, W2, S2>();
  auto lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return SymBitVec<kResultWidth, true>(lhs_ext - rhs_ext);
}

// Multiplication: result width = W1 + W2 (full product). Result is signed if
// either operand is signed.
template <size_t W1, bool S1, size_t W2, bool S2>
auto operator*(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  constexpr size_t kResultWidth = W1 + W2;
  constexpr bool kResultSigned = S1 || S2;
  auto lhs_ext = internal::extend<kResultWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kResultWidth, W2, S2>(rhs);
  return SymBitVec<kResultWidth, kResultSigned>(lhs_ext * rhs_ext);
}

// --- Unary negate (bit-growth) ---
// Result width = W + 1, always signed. Consistent with binary subtraction.

template <size_t W, bool S>
SymBitVec<W + 1, true> operator-(const SymBitVec<W, S>& val) {
  auto ext = internal::extend<W + 1, W, S>(val);
  return SymBitVec<W + 1, true>(-ext);
}

// --- Equality (strict: same width and signedness) ---

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator==(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  static_assert(W1 == W2 && S1 == S2,
                "comparison requires matching width and signedness; use "
                "z3w::math_eq(a, b) for mathematical equality, or cast one "
                "operand to the other's type");
  return SymBool(lhs.expr() == rhs.expr());
}

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator!=(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  static_assert(W1 == W2 && S1 == S2,
                "comparison requires matching width and signedness; use "
                "z3w::math_ne(a, b) for mathematical inequality, or cast one "
                "operand to the other's type");
  return SymBool(lhs.expr() != rhs.expr());
}

// --- Mathematical equality (heterogeneous: any width/signedness combination) ---

// math_eq asks whether two operands represent the same mathematical value.
// Both operands are widened to a common width that preserves every value from
// both sides before equality is checked. Works for any combination of width
// and signedness on the two operands.
template <size_t W1, bool S1, size_t W2, bool S2>
SymBool math_eq(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  constexpr size_t kCommonWidth =
      (S1 == S2) ? std::max(W1, W2) : std::max(W1, W2) + 1;
  auto lhs_ext = internal::extend<kCommonWidth, W1, S1>(lhs);
  auto rhs_ext = internal::extend<kCommonWidth, W2, S2>(rhs);
  return SymBool(lhs_ext == rhs_ext);
}

// math_ne is the negation of math_eq. See math_eq for semantics.
template <size_t W1, bool S1, size_t W2, bool S2>
SymBool math_ne(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  return !math_eq(lhs, rhs);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto math_eq(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return math_eq(internal::promote(lhs, ctx), internal::promote(rhs, ctx));
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto math_ne(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return math_ne(internal::promote(lhs, ctx), internal::promote(rhs, ctx));
}

// --- Mathematical ordering (heterogeneous: any width/signedness combination) ---

// math_lt asks whether the first operand is mathematically less than the
// second. Both operands are widened to a common width that preserves every
// value from both sides before comparing: same signedness uses
// max(W1, W2); mixed signedness uses max(W1, W2) + 1 so the unsigned
// operand's full range fits in the signed domain. The comparison itself
// uses z3::ult when both operands are unsigned, z3::slt otherwise.
// math_le, math_gt, and math_ge are derived from math_lt.
template <size_t W1, bool S1, size_t W2, bool S2>
SymBool math_lt(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
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

// math_le(a, b) is equivalent to !math_lt(b, a). See math_lt for semantics.
template <size_t W1, bool S1, size_t W2, bool S2>
SymBool math_le(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  return !math_lt(rhs, lhs);
}

// math_gt(a, b) is equivalent to math_lt(b, a). See math_lt for semantics.
template <size_t W1, bool S1, size_t W2, bool S2>
SymBool math_gt(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  return math_lt(rhs, lhs);
}

// math_ge(a, b) is equivalent to !math_lt(a, b). See math_lt for semantics.
template <size_t W1, bool S1, size_t W2, bool S2>
SymBool math_ge(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  return !math_lt(lhs, rhs);
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto math_lt(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return math_lt(internal::promote(lhs, ctx), internal::promote(rhs, ctx));
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto math_le(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return math_le(internal::promote(lhs, ctx), internal::promote(rhs, ctx));
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto math_gt(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return math_gt(internal::promote(lhs, ctx), internal::promote(rhs, ctx));
}

template <typename L, typename R>
  requires mixed_operands<L, R>
auto math_ge(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return math_ge(internal::promote(lhs, ctx), internal::promote(rhs, ctx));
}

// --- Ordered comparison (strict: same width and signedness) ---

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator<(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  static_assert(W1 == W2 && S1 == S2,
                "comparison requires matching width and signedness; use "
                "z3w::math_lt(a, b) for mathematical comparison, or cast one "
                "operand to the other's type");
  if constexpr (S1) {
    return SymBool(z3::slt(lhs.expr(), rhs.expr()));
  } else {
    return SymBool(z3::ult(lhs.expr(), rhs.expr()));
  }
}

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator<=(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  static_assert(W1 == W2 && S1 == S2,
                "comparison requires matching width and signedness; use "
                "z3w::math_le(a, b) for mathematical comparison, or cast one "
                "operand to the other's type");
  return !(rhs < lhs);
}

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator>(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  static_assert(W1 == W2 && S1 == S2,
                "comparison requires matching width and signedness; use "
                "z3w::math_gt(a, b) for mathematical comparison, or cast one "
                "operand to the other's type");
  return rhs < lhs;
}

template <size_t W1, bool S1, size_t W2, bool S2>
SymBool operator>=(const SymBitVec<W1, S1>& lhs, const SymBitVec<W2, S2>& rhs) {
  static_assert(W1 == W2 && S1 == S2,
                "comparison requires matching width and signedness; use "
                "z3w::math_ge(a, b) for mathematical comparison, or cast one "
                "operand to the other's type");
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
auto operator*(const L& lhs, const R& rhs) {
  auto& ctx = internal::get_ctx(lhs, rhs);
  return internal::promote(lhs, ctx) * internal::promote(rhs, ctx);
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

// --- Conditional selection (ite) ---

// SymBool condition with symbolic values.
template <size_t W, bool S>
SymBitVec<W, S> ite(const SymBool& cond, const SymBitVec<W, S>& true_val,
                    const SymBitVec<W, S>& false_val) {
  return SymBitVec<W, S>(
      z3::ite(cond.expr(), true_val.expr(), false_val.expr()));
}

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

// SymBool choices.
inline SymBool ite(const SymBool& cond, const SymBool& true_val,
                   const SymBool& false_val) {
  return SymBool(z3::ite(cond.expr(), true_val.expr(), false_val.expr()));
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
// Out-of-range offsets yield zero bits (zero-extension semantics).
template <size_t TargetWidth, size_t W, bool S, size_t IdxW>
SymUInt<TargetWidth> extract(const SymBitVec<W, S>& val,
                             const SymUInt<IdxW>& start_idx) {
  static_assert(TargetWidth > 0, "extract: TargetWidth must be > 0.");
  static_assert(TargetWidth <= W,
                "extract: TargetWidth must be <= input width.");
  // Match widths for the shift. When IdxW > W, zero-extend the source so that
  // large offsets correctly shift in zeros rather than wrapping.
  constexpr size_t ShiftW = std::max(W, IdxW);
  z3::expr wide_val =
      (ShiftW > W) ? z3::zext(val.expr(), ShiftW - W) : val.expr();
  z3::expr wide_idx = (ShiftW > IdxW)
                          ? z3::zext(start_idx.expr(), ShiftW - IdxW)
                          : start_idx.expr();
  auto shifted = z3::lshr(wide_val, wide_idx);
  return SymUInt<TargetWidth>(shifted.extract(TargetWidth - 1, 0));
}

// --- Bit replacement (replace) ---

// Static replacement: replace<LO>(src, field) -> SymBitVec<WS, S>.
// Replaces bits [LO+WF-1 : LO] in src with field.
template <size_t LO, size_t WS, bool S, size_t WF>
SymBitVec<WS, S> replace(const SymBitVec<WS, S>& src,
                         const SymUInt<WF>& field) {
  static_assert(WS >= LO + WF, "replace: field doesn't fit at given offset.");

  constexpr size_t kHigh = LO + WF;  // one past the top bit of field

  z3::expr result = field.expr();

  // Prepend high bits if any.
  if constexpr (kHigh < WS) {
    result = z3::concat(src.expr().extract(WS - 1, kHigh), result);
  }

  // Append low bits if any.
  if constexpr (LO > 0) {
    result = z3::concat(result, src.expr().extract(LO - 1, 0));
  }

  return SymBitVec<WS, S>(result);
}

// Symbolic-offset replacement: replace(src, field, lo) -> SymBitVec<WS, S>.
// Replaces WF bits starting at symbolic offset lo.
// Replacement bits beyond the source width have no effect.
template <size_t WS, bool S, size_t WF, size_t WL>
SymBitVec<WS, S> replace(const SymBitVec<WS, S>& src, const SymUInt<WF>& field,
                         const SymUInt<WL>& lo) {
  static_assert(WS >= WF, "replace: field wider than source.");

  auto& ctx = src.expr().ctx();

  // Match widths for the shift. When WL > WS, zero-extend the source and mask
  // so that large offsets correctly shift field bits out rather than wrapping.
  constexpr size_t ShiftW = std::max(WS, WL);

  z3::expr wide_src =
      (ShiftW > WS) ? z3::zext(src.expr(), ShiftW - WS) : src.expr();
  z3::expr wide_field = z3::zext(field.expr(), ShiftW - WF);
  z3::expr wide_lo =
      (ShiftW > WL) ? z3::zext(lo.expr(), ShiftW - WL) : lo.expr();

  // Create WF-bit mask of all ones, extend to ShiftW.
  z3::expr mask = z3::zext(~ctx.bv_val(0, WF), ShiftW - WF);

  // Shift field and mask to position.
  z3::expr field_shifted = z3::shl(wide_field, wide_lo);
  z3::expr mask_shifted = z3::shl(mask, wide_lo);

  // Clear target bits and insert field.
  z3::expr wide_result = (wide_src & ~mask_shifted) | field_shifted;

  // Truncate back to source width.
  z3::expr result =
      (ShiftW > WS) ? wide_result.extract(WS - 1, 0) : wide_result;
  return SymBitVec<WS, S>(result);
}

// --- Concatenation ---

namespace internal {

// Normalize a concat argument to a z3::expr bit-vector.
template <size_t W, bool S>
z3::expr to_bv_expr(const SymBitVec<W, S>& val) {
  return val.expr();
}

inline z3::expr to_bv_expr(const SymBool& b) { return as_uint1(b).expr(); }

// Bit width of a symbolic type.
template <typename T>
struct sym_width;

template <size_t W, bool S>
struct sym_width<SymBitVec<W, S>> {
  static constexpr size_t value = W;
};

template <>
struct sym_width<SymBool> {
  static constexpr size_t value = 1;
};

template <typename T>
inline constexpr size_t sym_width_v = sym_width<T>::value;

}  // namespace internal

// concat(a, b): result width = width(a) + width(b), always SymUInt.
// Accepts SymBitVec (any signedness) and SymBool operands.
template <typename A, typename B>
  requires(is_symbolic_v<A> && is_symbolic_v<B>)
auto concat(const A& high, const B& low) {
  constexpr size_t kResultWidth =
      internal::sym_width_v<A> + internal::sym_width_v<B>;
  return SymUInt<kResultWidth>(
      z3::concat(internal::to_bv_expr(high), internal::to_bv_expr(low)));
}

// Variadic concat.
template <typename A, typename B, typename... Rest>
  requires(is_symbolic_v<A> && is_symbolic_v<B>)
auto concat(const A& high, const B& next, const Rest&... rest) {
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

// --- Left rotation ---

// Constant rotation: result preserves type.
template <size_t N, size_t W, bool S>
SymBitVec<W, S> rotl(const SymBitVec<W, S>& val) {
  return SymBitVec<W, S>(val.expr().rotate_left(N));
}

// Symbolic rotation: amount width may differ from value width.
template <size_t W, bool S, size_t K>
SymBitVec<W, S> rotl(const SymBitVec<W, S>& val,
                     const BitVec<K, false>& amount) {
  return rotl(val, to_symbolic(amount, val.expr().ctx()));
}

template <size_t W, bool S, size_t K>
SymBitVec<W, S> rotl(const SymBitVec<W, S>& val, const SymUInt<K>& amount) {
  auto amt_ext = unsafe_cast<SymUInt<W>>(amount);
  auto& ctx = val.expr().ctx();
  z3::expr r(ctx, Z3_mk_ext_rotate_left(ctx, val.expr(), amt_ext.expr()));
  ctx.check_error();
  return SymBitVec<W, S>(r);
}

// --- Right rotation ---

// Constant rotation: result preserves type.
template <size_t N, size_t W, bool S>
SymBitVec<W, S> rotr(const SymBitVec<W, S>& val) {
  return SymBitVec<W, S>(val.expr().rotate_right(N));
}

// Symbolic rotation: amount width may differ from value width.
template <size_t W, bool S, size_t K>
SymBitVec<W, S> rotr(const SymBitVec<W, S>& val,
                     const BitVec<K, false>& amount) {
  return rotr(val, to_symbolic(amount, val.expr().ctx()));
}

template <size_t W, bool S, size_t K>
SymBitVec<W, S> rotr(const SymBitVec<W, S>& val, const SymUInt<K>& amount) {
  auto amt_ext = unsafe_cast<SymUInt<W>>(amount);
  auto& ctx = val.expr().ctx();
  z3::expr r(ctx, Z3_mk_ext_rotate_right(ctx, val.expr(), amt_ext.expr()));
  ctx.check_error();
  return SymBitVec<W, S>(r);
}

}  // namespace z3w

#endif  // Z3WIRE_SYM_BIT_VEC_H_
