#ifndef Z3WIRE_BITFIELD_H_
#define Z3WIRE_BITFIELD_H_

#include <cstddef>

#include "z3wire/bitvec.h"
#include "z3wire/bool.h"

namespace z3w {

namespace internal {

// Width contribution of each field type.
template <typename T>
constexpr size_t field_width() {
  return T::kWidth;
}

// Bool has width 1.
template <>
constexpr size_t field_width<Bool>() {
  return 1;
}

// Convert a field to Ubv for concatenation. Returns by value.
template <size_t W, bool S>
Ubv<W> to_ubv_field(const BitVec<W, S>& field) {
  return cast<Ubv<W>>(field);
}

// Bool -> Ubv<1> conversion.
inline Ubv<1> to_ubv_field(const Bool& field) { return to_ubv1(field); }

// Base case: single field.
template <typename Field>
auto concat_fields(const Field& field) {
  return to_ubv_field(field);
}

// Recursive case: concat fields in reverse order for LSB-first layout.
// We build the concat from right to left: concat(rest..., first).
template <typename First, typename... Rest>
auto concat_fields(const First& first, const Rest&... rest) {
  return concat(concat_fields(rest...), to_ubv_field(first));
}

}  // namespace internal

// Returns a Bool constraint: buffer == concat of fields (LSB-first).
// The first field occupies the lowest bits, the last field the highest.
// static_assert verifies that field widths sum to the buffer width.
template <size_t W, typename... Fields>
Bool bitfield_eq(const Ubv<W>& buffer, const Fields&... fields) {
  static_assert((... + internal::field_width<Fields>()) == W,
                "Field widths must sum to the buffer width.");
  auto combined = internal::concat_fields(fields...);
  return buffer == combined;
}

}  // namespace z3w

#endif  // Z3WIRE_BITFIELD_H_
