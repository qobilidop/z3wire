#ifndef Z3WIRE_BOOL_H_
#define Z3WIRE_BOOL_H_

#include <ostream>

namespace z3w {

// Type-safe concrete boolean. Only accepts bool — rejects integers, floats,
// pointers, and all other types.
class Bool {
 public:
  constexpr Bool() : value_(false) {}
  constexpr Bool(bool v) : value_(v) {}  // NOLINT: implicit conversion intended
  template <typename T>
  Bool(T) = delete;

  explicit constexpr operator bool() const { return value_; }
  [[nodiscard]] constexpr bool value() const { return value_; }

  friend constexpr bool operator==(Bool lhs, Bool rhs) {
    return lhs.value_ == rhs.value_;
  }

  friend std::ostream& operator<<(std::ostream& os, Bool b) {
    return os << (b.value_ ? "true" : "false");
  }

 private:
  bool value_;
};

}  // namespace z3w

#endif  // Z3WIRE_BOOL_H_
