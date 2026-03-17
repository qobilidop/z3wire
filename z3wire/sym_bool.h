#ifndef Z3WIRE_SYM_BOOL_H_
#define Z3WIRE_SYM_BOOL_H_

#include <optional>
#include <string>

#include <z3++.h>

#include "z3wire/bool.h"

namespace z3w {

// Type-safe wrapper around a Z3 Bool expression.
class SymBool {
 public:
  // Default constructor: creates an uninitialized SymBool.
  SymBool() = default;

  // Create a symbolic boolean variable.
  explicit SymBool(z3::context& ctx, const std::string& name)
      : expr_(ctx.bool_const(name.c_str())) {}

  // Create a SymBool from a raw z3::expr. The caller must ensure it has Bool
  // sort.
  explicit SymBool(z3::expr expr) : expr_(std::move(expr)) {}

  // Boolean literals.
  static SymBool True(z3::context& ctx) { return SymBool(ctx.bool_val(true)); }
  static SymBool False(z3::context& ctx) {
    return SymBool(ctx.bool_val(false));
  }

  // Access the underlying z3::expr.
  [[nodiscard]] const z3::expr& expr() const { return *expr_; }

  // Logical operators.
  friend SymBool operator&&(const SymBool& lhs, const SymBool& rhs) {
    return SymBool(*lhs.expr_ && *rhs.expr_);
  }

  friend SymBool operator||(const SymBool& lhs, const SymBool& rhs) {
    return SymBool(*lhs.expr_ || *rhs.expr_);
  }

  friend SymBool operator^(const SymBool& lhs, const SymBool& rhs) {
    return SymBool(*lhs.expr_ != *rhs.expr_);  // XOR is boolean inequality.
  }

  SymBool operator!() const { return SymBool(!*expr_); }

  // Equality.
  friend SymBool operator==(const SymBool& lhs, const SymBool& rhs) {
    return SymBool(*lhs.expr_ == *rhs.expr_);
  }

  friend SymBool operator!=(const SymBool& lhs, const SymBool& rhs) {
    return SymBool(*lhs.expr_ != *rhs.expr_);
  }

 private:
  std::optional<z3::expr> expr_;
};

// Convert a concrete Bool to a symbolic SymBool.
inline SymBool to_symbolic(Bool val, z3::context& ctx) {
  return val.value() ? SymBool::True(ctx) : SymBool::False(ctx);
}

}  // namespace z3w

#endif  // Z3WIRE_SYM_BOOL_H_
