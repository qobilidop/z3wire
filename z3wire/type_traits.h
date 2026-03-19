#ifndef Z3WIRE_TYPE_TRAITS_H_
#define Z3WIRE_TYPE_TRAITS_H_

#include <cstddef>
#include <type_traits>

namespace z3w {

// Forward declarations allow trait specializations without including the full
// type headers. This avoids circular includes: sym_bit_vec.h uses these traits
// internally (for mixed_operands), so type_traits.h cannot include
// sym_bit_vec.h. Forward declarations break the cycle.
class Bool;
template <size_t W, bool IsSigned>
class BitVec;
class SymBool;
template <size_t W, bool IsSigned>
class SymBitVec;

// Type trait: is this a concrete Z3Wire type?
// Matches Bool, BitVec<W, S> (and its aliases UInt<W> and SInt<W>).
template <typename T>
struct is_concrete : std::false_type {};

template <>
struct is_concrete<Bool> : std::true_type {};

template <size_t W, bool S>
struct is_concrete<BitVec<W, S>> : std::true_type {};

template <typename T>
inline constexpr bool is_concrete_v = is_concrete<T>::value;

// Type trait: is this a symbolic Z3Wire type?
// Matches SymBool, SymBitVec<W, S> (and its aliases SymUInt<W> and SymSInt<W>).
template <typename T>
struct is_symbolic : std::false_type {};

template <>
struct is_symbolic<SymBool> : std::true_type {};

template <size_t W, bool S>
struct is_symbolic<SymBitVec<W, S>> : std::true_type {};

template <typename T>
inline constexpr bool is_symbolic_v = is_symbolic<T>::value;

}  // namespace z3w

#endif  // Z3WIRE_TYPE_TRAITS_H_
