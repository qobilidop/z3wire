#ifndef Z3WIRE_BOOL_H_
#define Z3WIRE_BOOL_H_

#include <string>

#include <z3++.h>

namespace z3w {

// Type-safe wrapper around a Z3 Bool expression.
class Bool {
 public:
  // Create a symbolic boolean variable.
  explicit Bool(z3::context& ctx, const std::string& name)
      : expr_(ctx.bool_const(name.c_str())) {}

  // Create a Bool from a raw z3::expr. The caller must ensure it has Bool sort.
  explicit Bool(z3::expr expr) : expr_(std::move(expr)) {}

  // Boolean literals.
  static Bool True(z3::context& ctx) { return Bool(ctx.bool_val(true)); }
  static Bool False(z3::context& ctx) { return Bool(ctx.bool_val(false)); }

  // Access the underlying z3::expr.
  [[nodiscard]] const z3::expr& raw() const { return expr_; }

  // Logical operators.
  friend Bool operator&&(const Bool& lhs, const Bool& rhs) {
    return Bool(lhs.expr_ && rhs.expr_);
  }

  friend Bool operator||(const Bool& lhs, const Bool& rhs) {
    return Bool(lhs.expr_ || rhs.expr_);
  }

  Bool operator!() const { return Bool(!expr_); }

  // Equality.
  friend Bool operator==(const Bool& lhs, const Bool& rhs) {
    return Bool(lhs.expr_ == rhs.expr_);
  }

  friend Bool operator!=(const Bool& lhs, const Bool& rhs) {
    return Bool(lhs.expr_ != rhs.expr_);
  }

 private:
  z3::expr expr_;
};

}  // namespace z3w

#endif  // Z3WIRE_BOOL_H_
