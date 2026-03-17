# Test Coverage

Line coverage is not a useful metric for a template-heavy library like Z3Wire.
Different template instantiations (e.g., `SymUInt<8> + SymUInt<8>` vs `SymSInt<8> + SymUInt<4>`)
exercise different `if constexpr` branches from the same source lines. This
checklist tracks which template instantiations are tested.

## Dimensions

Each operation can be instantiated along these dimensions:

| Dimension   | Values                                             |
| :---------- | :------------------------------------------------- |
| Type layer  | Concrete (`BitVec`), Symbolic (`SymBitVec`), Mixed |
| Signedness  | Unsigned (U), Signed (S), Mixed (U+S)              |
| Width match | Same width, Different widths                       |
| Boundary    | W=1, W=64 (concrete only, since W≤64)              |

Not every dimension applies to every operation. The tables below list the
relevant combinations and the test that covers each.

## Construction

### Raw constructor / symbolic variable

| Case                      | Test                                      |
| :------------------------ | :---------------------------------------- |
| Concrete unsigned         | `BitVecTest::RawConstructorMasks`         |
| Concrete unsigned non-2^N | `BitVecTest::RawConstructorNonPowerOfTwo` |
| Concrete signed           | `BitVecTest::RawConstructorSigned`        |
| Concrete W=1              | `BitVecTest::Width1`                      |
| Concrete W=64             | `BitVecTest::Width64Construction`         |
| Symbolic unsigned         | `SymBitVecTest::SymbolicVariable`         |
| Symbolic signed           | —                                         |
| Symbolic W=1              | —                                         |
| Symbolic W=64             | —                                         |

### Literal (compile-time checked)

| Case                     | Test                                |
| :----------------------- | :---------------------------------- |
| Concrete unsigned        | `BitVecTest::LiteralUnsigned`       |
| Concrete unsigned zero   | `BitVecTest::LiteralZero`           |
| Concrete signed positive | `BitVecTest::LiteralSigned`         |
| Concrete signed negative | `BitVecTest::LiteralSignedNegative` |
| Concrete W=64            | —                                   |
| Symbolic unsigned        | `SymBitVecTest::Literal`            |
| Symbolic unsigned zero   | `SymBitVecTest::LiteralZero`        |
| Symbolic signed          | —                                   |
| Symbolic W=64            | —                                   |

### Checked constructor

| Case                               | Test                                        |
| :--------------------------------- | :------------------------------------------ |
| Concrete U, no truncation          | `BitVecTest::CheckedNoTruncation`           |
| Concrete U, with truncation        | `BitVecTest::CheckedWithTruncation`         |
| Concrete S (int64_t), in range     | `BitVecTest::SignedCheckedNoTruncation`     |
| Concrete S (int64_t), positive max | `BitVecTest::SignedCheckedPositive`         |
| Concrete S (int64_t), overflow -   | `BitVecTest::SignedCheckedOverflowNegative` |
| Concrete S (int64_t), overflow +   | `BitVecTest::SignedCheckedOverflowPositive` |
| Concrete W=64                      | —                                           |

## Bitwise operators (`&`, `|`, `^`, `~`)

Bitwise operators require strict width and signedness matching.

| Case                   | Test                                  |
| :--------------------- | :------------------------------------ |
| Concrete U AND         | `BitVecTest::BitwiseAnd`              |
| Concrete U OR          | `BitVecTest::BitwiseOr`               |
| Concrete U XOR         | `BitVecTest::BitwiseXor`              |
| Concrete U NOT         | `BitVecTest::BitwiseNot`              |
| Concrete U NOT non-2^N | `BitVecTest::BitwiseNotNonPowerOfTwo` |
| Concrete S             | —                                     |
| Concrete W=1           | —                                     |
| Concrete W=64          | —                                     |
| Symbolic U AND         | `SymBitVecTest::BitwiseAnd`           |
| Symbolic U OR          | `SymBitVecTest::BitwiseOr`            |
| Symbolic U XOR         | `SymBitVecTest::BitwiseXor`           |
| Symbolic U NOT         | `SymBitVecTest::BitwiseNot`           |
| Symbolic S             | —                                     |
| Mixed AND              | `SymBitVecTest::MixedBitwiseAnd`      |
| Mixed OR               | —                                     |
| Mixed XOR              | —                                     |

## Equality (`==`, `!=`)

Equality allows different widths and signedness (relaxed).

| Case                         | Test                                                                                    |
| :--------------------------- | :-------------------------------------------------------------------------------------- |
| Concrete U, same width       | `BitVecTest::Equality`, `Inequality`                                                    |
| Concrete U, different widths | `BitVecTest::CrossTypeEqualitySameSignedness`, `CrossTypeEqualityDifferentValues`       |
| Concrete mixed sign          | `BitVecTest::CrossTypeEqualityMixedSignedness`                                          |
| Concrete S, same width       | —                                                                                       |
| Concrete W=1                 | —                                                                                       |
| Concrete W=64                | —                                                                                       |
| Symbolic U, same width       | `SymBitVecTest::Equality`, `Inequality`                                                 |
| Symbolic U, different widths | `SymBitVecTest::CrossTypeEqualityDifferentWidths`, `CrossTypeInequalityDifferentWidths` |
| Symbolic S, same width       | —                                                                                       |
| Symbolic mixed sign          | —                                                                                       |
| Mixed, same type             | `SymBitVecTest::MixedEquality`                                                          |
| Mixed, different types       | `SymBitVecTest::MixedCrossTypeEquality`                                                 |

## Ordered comparison (`<`, `<=`, `>`, `>=`)

Ordered comparison allows different widths and signedness (relaxed).

| Case                            | Test                                                                                               |
| :------------------------------ | :------------------------------------------------------------------------------------------------- |
| Concrete U, same width          | `BitVecTest::UnsignedLessThan`, `UnsignedGreaterThan`, `UnsignedLessEqual`, `UnsignedGreaterEqual` |
| Concrete S, same width          | `BitVecTest::SignedLessThan`, `SignedGreaterThan`                                                  |
| Concrete S, non-2^N width       | `BitVecTest::SignedComparisonNonPowerOfTwo`                                                        |
| Concrete U, different widths    | `BitVecTest::CrossTypeLessThanDifferentWidths`                                                     |
| Concrete mixed sign, same width | `BitVecTest::CrossTypeDifferentSignednessSameWidth`                                                |
| Concrete mixed sign, diff width | `BitVecTest::CrossTypeDifferentSignednessAndWidth`                                                 |
| Concrete U needs extra sign bit | `BitVecTest::CrossTypeUnsignedNeedsExtraSignBit`                                                   |
| Concrete S \<=, >=              | —                                                                                                  |
| Concrete W=1                    | `BitVecTest::Width1` (>)                                                                           |
| Concrete W=64                   | `BitVecTest::Width64SignedComparison`                                                              |
| Symbolic U, same width          | `SymBitVecTest::UnsignedLessThan`, `UnsignedGreaterThan`                                           |
| Symbolic S, same width          | `SymBitVecTest::SignedLessThan`                                                                    |
| Symbolic S \<=, >, >=           | —                                                                                                  |
| Symbolic U, different widths    | `SymBitVecTest::CrossTypeLessThanDifferentWidths`                                                  |
| Symbolic mixed sign             | `SymBitVecTest::CrossTypeLessThanDifferentSignedness`, `CrossTypeGreaterEqualDifferentSignedness`  |
| Mixed, same type                | `SymBitVecTest::MixedLessThan`                                                                     |
| Mixed, different types          | `SymBitVecTest::MixedCrossTypeLessThan`                                                            |

## Arithmetic (`+`, `-`)

Arithmetic uses bit-growth: result width = `max(W1, W2) + 1`.

### Addition

| Case                            | Test                                                  |
| :------------------------------ | :---------------------------------------------------- |
| Concrete UU, same width         | `BitVecTest::AdditionWidens`                          |
| Concrete UU, different widths   | `BitVecTest::AdditionDifferentWidths`                 |
| Concrete mixed sign             | `BitVecTest::AdditionMixedSignedness`                 |
| Concrete SS                     | —                                                     |
| Concrete W=1                    | —                                                     |
| Concrete W=64 (→ W=65, invalid) | N/A (concrete W≤64)                                   |
| Concrete W=63 (→ W=64)          | `BitVecTest::Width64Arithmetic`                       |
| Symbolic UU, same width         | `SymBitVecTest::AdditionWidens`, `AdditionNoOverflow` |
| Symbolic UU, different widths   | `SymBitVecTest::AdditionDifferentWidths`              |
| Symbolic UU, chained            | `SymBitVecTest::AdditionChained`                      |
| Symbolic mixed sign             | `SymBitVecTest::AdditionMixedSignedness`              |
| Symbolic SS                     | —                                                     |
| Mixed sym+conc                  | `SymBitVecTest::MixedAddSymbolicPlusConcrete`         |
| Mixed conc+sym                  | `SymBitVecTest::MixedAddConcretePlusSymbolic`         |
| Mixed different widths          | `SymBitVecTest::MixedAddDifferentWidths`              |

### Subtraction

| Case                         | Test                                           |
| :--------------------------- | :--------------------------------------------- |
| Concrete UU, positive result | `BitVecTest::SubtractionPositiveResult`        |
| Concrete UU, negative result | `BitVecTest::SubtractionIsSigned`              |
| Concrete mixed sign          | —                                              |
| Concrete SS                  | —                                              |
| Symbolic UU, type check      | `SymBitVecTest::SubtractionIsSigned`           |
| Symbolic UU, value check     | `SymBitVecTest::SubtractionCorrectValue`       |
| Symbolic mixed sign          | —                                              |
| Symbolic SS                  | —                                              |
| Mixed sym-conc               | `SymBitVecTest::MixedSubSymbolicMinusConcrete` |
| Mixed conc-sym               | —                                              |

## Shifts (`shl`, `shr`)

### Left shift (`shl`)

| Case                          | Test                               |
| :---------------------------- | :--------------------------------- |
| Symbolic constant N           | `SymBitVecTest::ShlConstant`       |
| Symbolic dynamic amount       | `SymBitVecTest::ShlSymbolic`       |
| Shl with truncation (compose) | `SymBitVecTest::ShlWithTruncation` |

### Right shift (`shr`)

| Case                    | Test                         |
| :---------------------- | :--------------------------- |
| Symbolic U (logical)    | `SymBitVecTest::ShrUnsigned` |
| Symbolic S (arithmetic) | `SymBitVecTest::ShrSigned`   |

### Signedness reinterpretation

| Case          | Test                        |
| :------------ | :-------------------------- |
| `as_unsigned` | `SymBitVecTest::AsUnsigned` |
| `as_signed`   | `SymBitVecTest::AsSigned`   |

## Casting (`unsafe_cast`, `safe_cast`, `checked_cast`)

### unsafe_cast (raw)

| Case                      | Test                               |
| :------------------------ | :--------------------------------- |
| Concrete truncation       | `BitVecTest::CastTruncation`       |
| Concrete zero extension   | `BitVecTest::CastZeroExtension`    |
| Concrete sign extension   | `BitVecTest::CastSignExtension`    |
| Concrete bitcast (same W) | `BitVecTest::CastBitcast`          |
| Concrete W=1              | —                                  |
| Concrete W=64             | —                                  |
| Symbolic truncation       | `SymBitVecTest::CastTruncation`    |
| Symbolic zero extension   | `SymBitVecTest::CastZeroExtension` |
| Symbolic sign extension   | `SymBitVecTest::CastSignExtension` |
| Symbolic bitcast          | `SymBitVecTest::CastBitcast`       |

### safe_cast (compile-time lossless)

| Case                 | Test                                      |
| :------------------- | :---------------------------------------- |
| Concrete UU widening | `BitVecTest::SafeCastWidening`            |
| Concrete US widening | `BitVecTest::SafeCastUnsignedToSigned`    |
| Concrete SS widening | —                                         |
| Symbolic UU widening | `SymBitVecTest::SafeCastWidening`         |
| Symbolic US widening | `SymBitVecTest::SafeCastUnsignedToSigned` |
| Symbolic SS widening | —                                         |

### checked_cast (runtime value-preservation flag)

| Case                         | Test                                          |
| :--------------------------- | :-------------------------------------------- |
| Concrete value preserved     | `BitVecTest::CheckedCastNoOverflow`           |
| Concrete value not preserved | `BitVecTest::CheckedCastWithOverflow`         |
| Concrete cross-sign          | —                                             |
| Symbolic value preserved     | `SymBitVecTest::CheckedCastValuePreserved`    |
| Symbolic value not preserved | `SymBitVecTest::CheckedCastValueNotPreserved` |
| Symbolic cross-sign          | —                                             |

## Bit manipulation (`extract`, `concat`)

### extract

| Case                    | Test                             |
| :---------------------- | :------------------------------- |
| Concrete static         | `BitVecTest::StaticExtract`      |
| Concrete from signed    | —                                |
| Concrete W=1            | —                                |
| Symbolic static         | `SymBitVecTest::StaticExtract`   |
| Symbolic dynamic offset | `SymBitVecTest::SymbolicExtract` |
| Symbolic from signed    | —                                |

### concat

| Case                  | Test                            |
| :-------------------- | :------------------------------ |
| Concrete binary       | `BitVecTest::Concat`            |
| Concrete variadic     | `BitVecTest::ConcatVariadic`    |
| Concrete W=1 operands | —                               |
| Symbolic binary       | `SymBitVecTest::Concat`         |
| Symbolic variadic     | `SymBitVecTest::ConcatVariadic` |
| Symbolic W=1 operands | —                               |

## SymBool operations

| Case                   | Test                      |
| :--------------------- | :------------------------ |
| Literals (True, False) | `SymBoolTest::Literals`   |
| Logical AND            | `SymBoolTest::LogicalAnd` |
| Logical OR             | `SymBoolTest::LogicalOr`  |
| Logical NOT            | `SymBoolTest::LogicalNot` |
| Equality               | `SymBoolTest::Equality`   |
| Inequality             | `SymBoolTest::Inequality` |

## SymBool / bit-vector conversion

| Case                           | Test                             |
| :----------------------------- | :------------------------------- |
| Concrete Bool → UInt\<1>       | `BitVecTest::ToUInt1`            |
| Concrete UInt\<1> → Bool       | `BitVecTest::ToBoolFromUInt1`    |
| Symbolic SymBool → SymUInt\<1> | `SymBitVecTest::ToUbv1`          |
| Symbolic SymUInt\<1> → SymBool | `SymBitVecTest::ToBool`          |
| Symbolic roundtrip             | `SymBitVecTest::ToBoolRoundtrip` |

## Conditional selection (`ite`)

| Case                          | Test                                                |
| :---------------------------- | :-------------------------------------------------- |
| Concrete true branch          | `BitVecTest::IteTrue`                               |
| Concrete false branch         | `BitVecTest::IteFalse`                              |
| Concrete S                    | —                                                   |
| Symbolic SymBool condition    | `SymBitVecTest::Ite`                                |
| Symbolic bool condition       | —                                                   |
| Symbolic S                    | —                                                   |
| Mixed SymBool + concrete vals | `SymBitVecTest::MixedIteSymbolicCondConcreteValues` |
| Mixed SymBool + mixed vals    | `SymBitVecTest::MixedIteSymbolicCondMixedValues`    |

## Type traits

| Case                         | Test                            |
| :--------------------------- | :------------------------------ |
| `is_concrete_v<UInt<8>>`     | `BitVecTest::IsConcreteV`       |
| `is_concrete_v<SInt<16>>`    | `BitVecTest::IsConcreteV`       |
| `is_concrete_v<int>`         | `BitVecTest::IsConcreteV`       |
| `is_symbolic_v<SymUInt<8>>`  | `SymBitVecTest::IsSymbolicV`    |
| `is_symbolic_v<SymSInt<16>>` | `SymBitVecTest::IsSymbolicV`    |
| `is_symbolic_v<SymBool>`     | `SymBitVecTest::IsSymbolicBool` |
| `is_symbolic_v<int>`         | `SymBitVecTest::IsSymbolicV`    |

## Compile-time guards

Compile-fail tests verify that `static_assert` guards and SFINAE constraints
correctly reject invalid code. See [compile-fail tests](../design/compile-fail-tests.md)
for the test infrastructure.

### bit_vec.h guards (13 tests)

| Guard                                    | Test                                                   |
| :--------------------------------------- | :----------------------------------------------------- |
| `BitVec<0, S>` width too small           | `bit_vec_zero_width_test`                              |
| `BitVec<65, S>` width too large          | `bit_vec_width_too_large_test`                         |
| `SInt<8>::Literal<128>` above max        | `bit_vec_signed_literal_above_max_test`                |
| `SInt<8>::Literal<-129>` below min       | `bit_vec_signed_literal_below_min_test`                |
| `UInt<8>::Literal<-1>` below min         | `bit_vec_unsigned_literal_below_min_test`              |
| `UInt<8>::Literal<256>` above max        | `bit_vec_unsigned_literal_above_max_test`              |
| `safe_cast` signed → unsigned            | `bit_vec_safe_cast_signed_to_unsigned_test`            |
| `safe_cast` unsigned narrowing           | `bit_vec_safe_cast_unsigned_narrow_test`               |
| `safe_cast` signed narrowing             | `bit_vec_safe_cast_signed_narrow_test`                 |
| `safe_cast` unsigned → signed same width | `bit_vec_safe_cast_unsigned_to_signed_same_width_test` |
| `extract` High < Low                     | `bit_vec_extract_high_lt_low_test`                     |
| `extract` High >= width                  | `bit_vec_extract_out_of_bounds_test`                   |
| `checked(int64_t)` on unsigned (SFINAE)  | `bit_vec_checked_int64_on_unsigned_test`               |

### sym_bit_vec.h guards (11 tests)

| Guard                                    | Test                                                       |
| :--------------------------------------- | :--------------------------------------------------------- |
| `SymBitVec<0, S>` width zero             | `sym_bit_vec_zero_width_test`                              |
| `SymUInt<8>::Literal<256>` above max     | `sym_bit_vec_literal_above_max_test`                       |
| `internal::extend` target < source       | `sym_bit_vec_extend_shrink_test`                           |
| `safe_cast` signed → unsigned            | `sym_bit_vec_safe_cast_signed_to_unsigned_test`            |
| `safe_cast` unsigned narrowing           | `sym_bit_vec_safe_cast_unsigned_narrow_test`               |
| `safe_cast` signed narrowing             | `sym_bit_vec_safe_cast_signed_narrow_test`                 |
| `safe_cast` unsigned → signed same width | `sym_bit_vec_safe_cast_unsigned_to_signed_same_width_test` |
| `extract` High < Low                     | `sym_bit_vec_extract_high_lt_low_test`                     |
| `extract` High >= width                  | `sym_bit_vec_extract_out_of_bounds_test`                   |
| Symbolic `extract` zero width            | `sym_bit_vec_symbolic_extract_zero_width_test`             |
| Symbolic `extract` too wide              | `sym_bit_vec_symbolic_extract_too_wide_test`               |

## Concrete-to-symbolic promotion

| Case           | Test                                      |
| :------------- | :---------------------------------------- |
| UInt → SymUInt | `SymBitVecTest::ConcreteToSymbolic`       |
| SInt → SymSInt | `SymBitVecTest::SignedConcreteToSymbolic` |
| W=1            | —                                         |
| W=64           | —                                         |
