#ifndef Z3WIRE_BOOL_H_
#define Z3WIRE_BOOL_H_

#include <concepts>
#include <ostream>

namespace z3w {

// Type-safe concrete boolean. Prevents implicit construction from integers
// and other non-boolean types.
class Bool {
 public:
  constexpr Bool() : value_(false) {}
  constexpr Bool(bool v) : value_(v) {}  // NOLINT: implicit conversion intended
  template <std::integral T>
  Bool(T) = delete;

  explicit constexpr operator bool() const { return value_; }
  constexpr bool value() const { return value_; }

  friend constexpr bool operator==(Bool lhs, Bool rhs) {
    return lhs.value_ == rhs.value_;
  }
  friend constexpr bool operator!=(Bool lhs, Bool rhs) {
    return lhs.value_ != rhs.value_;
  }
  friend constexpr bool operator==(Bool lhs, bool rhs) {
    return lhs.value_ == rhs;
  }
  friend constexpr bool operator==(bool lhs, Bool rhs) {
    return lhs == rhs.value_;
  }

  friend std::ostream& operator<<(std::ostream& os, Bool b) {
    return os << (b.value_ ? "true" : "false");
  }

 private:
  bool value_;
};

}  // namespace z3w

#endif  // Z3WIRE_BOOL_H_
