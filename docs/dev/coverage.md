# Test Coverage Checklist

Line coverage is not a useful metric for a template-heavy library like Z3Wire.
Different template instantiations (e.g., `Ubv<8> + Ubv<8>` vs `Sbv<8> + Ubv<4>`)
exercise different `if constexpr` branches from the same source lines. This
checklist tracks which template instantiations are tested.

## Dimensions

Each operation can be instantiated along these dimensions:

| Dimension | Values |
| :----------- | :-------------------------------------------------- |
| Type layer | Concrete (`Int`), Symbolic (`BitVec`), Mixed |
| Signedness | Unsigned (U), Signed (S), Mixed (U+S) |
| Width match | Same width, Different widths |
| Boundary | W=1, W=64 (concrete only, since W≤64) |

Not every dimension applies to every operation. The tables below list the
relevant combinations and the test that covers each.

## Construction

### Raw constructor / symbolic variable

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete unsigned | `IntTest::RawConstructorMasks` |
| Concrete unsigned non-2^N | `IntTest::RawConstructorNonPowerOfTwo` |
| Concrete signed | `IntTest::RawConstructorSigned` |
| Concrete W=1 | `IntTest::Width1` |
| Concrete W=64 | `IntTest::Width64Construction` |
| Symbolic unsigned | `BitVecTest::SymbolicVariable` |
| Symbolic signed | — |
| Symbolic W=1 | — |
| Symbolic W=64 | — |

### Literal (compile-time checked)

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete unsigned | `IntTest::LiteralUnsigned` |
| Concrete unsigned zero | `IntTest::LiteralZero` |
| Concrete signed positive | `IntTest::LiteralSigned` |
| Concrete signed negative | `IntTest::LiteralSignedNegative` |
| Concrete W=64 | — |
| Symbolic unsigned | `BitVecTest::Literal` |
| Symbolic unsigned zero | `BitVecTest::LiteralZero` |
| Symbolic signed | — |
| Symbolic W=64 | — |

### Checked constructor

| Case | Test |
| :-------------------------- | :----------------------------------------- |
| Concrete U, no truncation | `IntTest::CheckedNoTruncation` |
| Concrete U, with truncation | `IntTest::CheckedWithTruncation` |
| Concrete S (int64_t), in range | `IntTest::SignedCheckedNoTruncation` |
| Concrete S (int64_t), positive max | `IntTest::SignedCheckedPositive` |
| Concrete S (int64_t), overflow - | `IntTest::SignedCheckedOverflowNegative` |
| Concrete S (int64_t), overflow + | `IntTest::SignedCheckedOverflowPositive` |
| Concrete W=64 | — |

## Bitwise operators (`&`, `|`, `^`, `~`)

Bitwise operators require strict width and signedness matching.

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete U AND | `IntTest::BitwiseAnd` |
| Concrete U OR | `IntTest::BitwiseOr` |
| Concrete U XOR | `IntTest::BitwiseXor` |
| Concrete U NOT | `IntTest::BitwiseNot` |
| Concrete U NOT non-2^N | `IntTest::BitwiseNotNonPowerOfTwo` |
| Concrete S | — |
| Concrete W=1 | — |
| Concrete W=64 | — |
| Symbolic U AND | `BitVecTest::BitwiseAnd` |
| Symbolic U OR | `BitVecTest::BitwiseOr` |
| Symbolic U XOR | `BitVecTest::BitwiseXor` |
| Symbolic U NOT | `BitVecTest::BitwiseNot` |
| Symbolic S | — |
| Mixed AND | `BitVecTest::MixedBitwiseAnd` |
| Mixed OR | — |
| Mixed XOR | — |

## Equality (`==`, `!=`)

Equality allows different widths and signedness (relaxed).

| Case | Test |
| :------------------------------- | :---------------------------------------------- |
| Concrete U, same width | `IntTest::Equality`, `Inequality` |
| Concrete U, different widths | `IntTest::CrossTypeEqualitySameSignedness`, `CrossTypeEqualityDifferentValues` |
| Concrete mixed sign | `IntTest::CrossTypeEqualityMixedSignedness` |
| Concrete S, same width | — |
| Concrete W=1 | — |
| Concrete W=64 | — |
| Symbolic U, same width | `BitVecTest::Equality`, `Inequality` |
| Symbolic U, different widths | `BitVecTest::CrossTypeEqualityDifferentWidths`, `CrossTypeInequalityDifferentWidths` |
| Symbolic S, same width | — |
| Symbolic mixed sign | — |
| Mixed, same type | `BitVecTest::MixedEquality` |
| Mixed, different types | `BitVecTest::MixedCrossTypeEquality` |

## Ordered comparison (`<`, `<=`, `>`, `>=`)

Ordered comparison allows different widths and signedness (relaxed).

| Case | Test |
| :------------------------------- | :---------------------------------------------- |
| Concrete U, same width | `IntTest::UnsignedLessThan`, `UnsignedGreaterThan`, `UnsignedLessEqual`, `UnsignedGreaterEqual` |
| Concrete S, same width | `IntTest::SignedLessThan`, `SignedGreaterThan` |
| Concrete S, non-2^N width | `IntTest::SignedComparisonNonPowerOfTwo` |
| Concrete U, different widths | `IntTest::CrossTypeLessThanDifferentWidths` |
| Concrete mixed sign, same width | `IntTest::CrossTypeDifferentSignednessSameWidth` |
| Concrete mixed sign, diff width | `IntTest::CrossTypeDifferentSignednessAndWidth` |
| Concrete U needs extra sign bit | `IntTest::CrossTypeUnsignedNeedsExtraSignBit` |
| Concrete S \<=, >= | — |
| Concrete W=1 | `IntTest::Width1` (>) |
| Concrete W=64 | `IntTest::Width64SignedComparison` |
| Symbolic U, same width | `BitVecTest::UnsignedLessThan`, `UnsignedGreaterThan` |
| Symbolic S, same width | `BitVecTest::SignedLessThan` |
| Symbolic S \<=, >, >= | — |
| Symbolic U, different widths | `BitVecTest::CrossTypeLessThanDifferentWidths` |
| Symbolic mixed sign | `BitVecTest::CrossTypeLessThanDifferentSignedness`, `CrossTypeGreaterEqualDifferentSignedness` |
| Mixed, same type | `BitVecTest::MixedLessThan` |
| Mixed, different types | `BitVecTest::MixedCrossTypeLessThan` |

## Arithmetic (`+`, `-`)

Arithmetic uses bit-growth: result width = `max(W1, W2) + 1`.

### Addition

| Case | Test |
| :------------------------------- | :---------------------------------------------- |
| Concrete UU, same width | `IntTest::AdditionWidens` |
| Concrete UU, different widths | `IntTest::AdditionDifferentWidths` |
| Concrete mixed sign | `IntTest::AdditionMixedSignedness` |
| Concrete SS | — |
| Concrete W=1 | — |
| Concrete W=64 (→ W=65, invalid) | N/A (concrete W≤64) |
| Concrete W=63 (→ W=64) | `IntTest::Width64Arithmetic` |
| Symbolic UU, same width | `BitVecTest::AdditionWidens`, `AdditionNoOverflow` |
| Symbolic UU, different widths | `BitVecTest::AdditionDifferentWidths` |
| Symbolic UU, chained | `BitVecTest::AdditionChained` |
| Symbolic mixed sign | `BitVecTest::AdditionMixedSignedness` |
| Symbolic SS | — |
| Mixed sym+conc | `BitVecTest::MixedAddSymbolicPlusConcrete` |
| Mixed conc+sym | `BitVecTest::MixedAddConcretePlusSymbolic` |
| Mixed different widths | `BitVecTest::MixedAddDifferentWidths` |

### Subtraction

| Case | Test |
| :------------------------------- | :---------------------------------------------- |
| Concrete UU, positive result | `IntTest::SubtractionPositiveResult` |
| Concrete UU, negative result | `IntTest::SubtractionIsSigned` |
| Concrete mixed sign | — |
| Concrete SS | — |
| Symbolic UU, type check | `BitVecTest::SubtractionIsSigned` |
| Symbolic UU, value check | `BitVecTest::SubtractionCorrectValue` |
| Symbolic mixed sign | — |
| Symbolic SS | — |
| Mixed sym-conc | `BitVecTest::MixedSubSymbolicMinusConcrete` |
| Mixed conc-sym | — |

## Shifts (`<<`, `>>`)

Hardware shifts require strict width and signedness matching.

### Hardware shifts

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete U \<< | `IntTest::LeftShift` |
| Concrete U \<< overflow | `IntTest::LeftShiftOverflow` |
| Concrete U >> | `IntTest::UnsignedRightShift` |
| Concrete S >> (arithmetic) | `IntTest::SignedRightShift` |
| Concrete S \<< | — |
| Concrete W=1 | — |
| Concrete W=64 | — |
| Symbolic U \<< | `BitVecTest::LeftShift` |
| Symbolic U >> | `BitVecTest::UnsignedRightShift` |
| Symbolic S >> (arithmetic) | `BitVecTest::SignedRightShift` |
| Symbolic S \<< | — |
| Mixed \<< | `BitVecTest::MixedLeftShift` |
| Mixed >> | `BitVecTest::MixedRightShift` |

### Checked shifts

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete checked_shl, no loss | `IntTest::CheckedShlNoLoss` |
| Concrete checked_shl, with loss | `IntTest::CheckedShlWithLoss` |
| Concrete checked_shr, with loss | `IntTest::CheckedShrWithLoss` |
| Concrete checked_shr, no loss | — |
| Concrete S checked_shl | — |
| Symbolic checked_shl, no loss | `BitVecTest::CheckedShlNoLoss` |
| Symbolic checked_shl, with loss | `BitVecTest::CheckedShlWithLoss` |
| Symbolic checked_shr, with loss | `BitVecTest::CheckedShrWithLoss` |
| Symbolic checked_shr, no loss | — |

### Lossless left shift

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete constant N | `IntTest::LosslessShlConstant` |
| Symbolic constant N | `BitVecTest::LosslessShlConstant` |
| Symbolic dynamic amount | `BitVecTest::LosslessShlSymbolic` |

## Casting (`cast`, `safe_cast`, `checked_cast`)

### cast (raw)

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete truncation | `IntTest::CastTruncation` |
| Concrete zero extension | `IntTest::CastZeroExtension` |
| Concrete sign extension | `IntTest::CastSignExtension` |
| Concrete bitcast (same W) | `IntTest::CastBitcast` |
| Concrete W=1 | — |
| Concrete W=64 | — |
| Symbolic truncation | `BitVecTest::CastTruncation` |
| Symbolic zero extension | `BitVecTest::CastZeroExtension` |
| Symbolic sign extension | `BitVecTest::CastSignExtension` |
| Symbolic bitcast | `BitVecTest::CastBitcast` |

### safe_cast (compile-time lossless)

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete UU widening | `IntTest::SafeCastWidening` |
| Concrete US widening | `IntTest::SafeCastUnsignedToSigned` |
| Concrete SS widening | — |
| Symbolic UU widening | `BitVecTest::SafeCastWidening` |
| Symbolic US widening | `BitVecTest::SafeCastUnsignedToSigned` |
| Symbolic SS widening | — |

### checked_cast (runtime overflow flag)

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete no overflow | `IntTest::CheckedCastNoOverflow` |
| Concrete with overflow | `IntTest::CheckedCastWithOverflow` |
| Concrete cross-sign | — |
| Symbolic no overflow | `BitVecTest::CheckedCastNoOverflow` |
| Symbolic with overflow | `BitVecTest::CheckedCastWithOverflow` |
| Symbolic cross-sign | — |

## Bit manipulation (`extract`, `concat`)

### extract

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete static | `IntTest::StaticExtract` |
| Concrete from signed | — |
| Concrete W=1 | — |
| Symbolic static | `BitVecTest::StaticExtract` |
| Symbolic dynamic offset | `BitVecTest::SymbolicExtract` |
| Symbolic from signed | — |

### concat

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete binary | `IntTest::Concat` |
| Concrete variadic | `IntTest::ConcatVariadic` |
| Concrete W=1 operands | — |
| Symbolic binary | `BitVecTest::Concat` |
| Symbolic variadic | `BitVecTest::ConcatVariadic` |
| Symbolic W=1 operands | — |

## Bool operations

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Literals (True, False) | `BoolTest::Literals` |
| Logical AND | `BoolTest::LogicalAnd` |
| Logical OR | `BoolTest::LogicalOr` |
| Logical NOT | `BoolTest::LogicalNot` |
| Equality | `BoolTest::Equality` |
| Inequality | `BoolTest::Inequality` |

## Bool / bit-vector conversion

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete bool → UInt\<1> | `IntTest::ToUInt1` |
| Concrete UInt\<1> → bool | `IntTest::ToBoolFromUInt1` |
| Symbolic Bool → Ubv\<1> | `BitVecTest::ToUbv1` |
| Symbolic Ubv\<1> → Bool | `BitVecTest::ToBool` |
| Symbolic roundtrip | `BitVecTest::ToBoolRoundtrip` |

## Conditional selection (`ite`)

| Case | Test |
| :-------------------------- | :------------------------------------- |
| Concrete true branch | `IntTest::IteTrue` |
| Concrete false branch | `IntTest::IteFalse` |
| Concrete S | — |
| Symbolic Bool condition | `BitVecTest::Ite` |
| Symbolic bool condition | — |
| Symbolic S | — |
| Mixed Bool + concrete vals | `BitVecTest::MixedIteSymbolicCondConcreteValues` |
| Mixed Bool + mixed vals | `BitVecTest::MixedIteSymbolicCondMixedValues` |

## Type traits

| Case | Test |
| :-------------------------- | :------------------------------------- |
| `is_concrete_v<UInt<8>>` | `IntTest::IsConcreteV` |
| `is_concrete_v<SInt<16>>` | `IntTest::IsConcreteV` |
| `is_concrete_v<int>` | `IntTest::IsConcreteV` |
| `is_symbolic_v<Ubv<8>>` | `BitVecTest::IsSymbolicV` |
| `is_symbolic_v<Sbv<16>>` | `BitVecTest::IsSymbolicV` |
| `is_symbolic_v<Bool>` | `BitVecTest::IsSymbolicBool` |
| `is_symbolic_v<int>` | `BitVecTest::IsSymbolicV` |

## Compile-time guards

Compile-fail tests verify that `static_assert` guards and SFINAE constraints
correctly reject invalid code. See [compile-fail tests](../design/compile-fail-tests.md)
for the test infrastructure.

### int.h guards (13 tests)

| Guard | Test |
| :---- | :--- |
| `Int<0, S>` width too small | `int_zero_width_test` |
| `Int<65, S>` width too large | `int_width_too_large_test` |
| `SInt<8>::Literal<128>` above max | `int_signed_literal_above_max_test` |
| `SInt<8>::Literal<-129>` below min | `int_signed_literal_below_min_test` |
| `UInt<8>::Literal<-1>` below min | `int_unsigned_literal_below_min_test` |
| `UInt<8>::Literal<256>` above max | `int_unsigned_literal_above_max_test` |
| `safe_cast` signed → unsigned | `int_safe_cast_signed_to_unsigned_test` |
| `safe_cast` unsigned narrowing | `int_safe_cast_unsigned_narrow_test` |
| `safe_cast` signed narrowing | `int_safe_cast_signed_narrow_test` |
| `safe_cast` unsigned → signed same width | `int_safe_cast_unsigned_to_signed_same_width_test` |
| `extract` High < Low | `int_extract_high_lt_low_test` |
| `extract` High >= width | `int_extract_out_of_bounds_test` |
| `checked(int64_t)` on unsigned (SFINAE) | `int_checked_int64_on_unsigned_test` |

### bitvec.h guards (11 tests)

| Guard | Test |
| :---- | :--- |
| `BitVec<0, S>` width zero | `bitvec_zero_width_test` |
| `Ubv<8>::Literal<256>` above max | `bitvec_literal_above_max_test` |
| `internal::extend` target < source | `bitvec_extend_shrink_test` |
| `safe_cast` signed → unsigned | `bitvec_safe_cast_signed_to_unsigned_test` |
| `safe_cast` unsigned narrowing | `bitvec_safe_cast_unsigned_narrow_test` |
| `safe_cast` signed narrowing | `bitvec_safe_cast_signed_narrow_test` |
| `safe_cast` unsigned → signed same width | `bitvec_safe_cast_unsigned_to_signed_same_width_test` |
| `extract` High < Low | `bitvec_extract_high_lt_low_test` |
| `extract` High >= width | `bitvec_extract_out_of_bounds_test` |
| Symbolic `extract` zero width | `bitvec_symbolic_extract_zero_width_test` |
| Symbolic `extract` too wide | `bitvec_symbolic_extract_too_wide_test` |

## Concrete-to-symbolic promotion

| Case | Test |
| :-------------------------- | :------------------------------------- |
| UInt → Ubv | `BitVecTest::ConcreteToSymbolic` |
| SInt → Sbv | `BitVecTest::SignedConcreteToSymbolic` |
| W=1 | — |
| W=64 | — |
