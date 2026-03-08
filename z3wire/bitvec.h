#ifndef Z3WIRE_BITVEC_H_
#define Z3WIRE_BITVEC_H_

#include <z3++.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "z3wire/bool.h"

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
    return BitVec(ctx.bv_val(static_cast<uint64_t>(Value), Width));
  }

  const z3::expr& raw() const { return expr_; }

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

// --- Conditional selection (ite) ---

template <size_t W, bool S>
BitVec<W, S> ite(const Bool& cond, const BitVec<W, S>& true_val,
                 const BitVec<W, S>& false_val) {
  return BitVec<W, S>(z3::ite(cond.raw(), true_val.raw(), false_val.raw()));
}

}  // namespace z3w

#endif  // Z3WIRE_BITVEC_H_
