# bitfield_eq Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement `z3w::bitfield_eq`, a function that returns a symbolic equality constraint between a bit-vector buffer and its LSB-first field decomposition.

**Architecture:** A single variadic function template in a new header (`z3wire/bitfield.h`). It converts each field to `Ubv` (handling `Bool` and `Sbv` automatically), concatenates them in reverse order (since `concat` is MSB-first but fields are LSB-first), and returns a `z3w::Bool` equality constraint. Compile-time `static_assert` ensures field widths sum to the buffer width.

**Tech Stack:** C++20 templates, Z3 C++ API, Google Test, Bazel

**Spec:** `docs/design/bitfield-eq.md`

______________________________________________________________________

## File structure

| File                                                            | Responsibility                          |
| :-------------------------------------------------------------- | :-------------------------------------- |
| Create: `z3wire/bitfield.h`                                     | `bitfield_eq` function template         |
| Create: `z3wire/bitfield_test.cc`                               | Unit tests                              |
| Create: `compile_fail_tests/bitfield_eq_width_mismatch_test.cc` | Compile-fail test for width mismatch    |
| Modify: `z3wire/BUILD.bazel`                                    | Add `bitfield` library and test targets |
| Modify: `compile_fail_tests/BUILD.bazel`                        | Add compile-fail test target            |
| Modify: `docs/usage/operations.md`                              | Add "Bit field" section                 |
| Modify: `docs/usage/cheatsheet.md`                              | Add `bitfield_eq` entry                 |

______________________________________________________________________

## Chunk 1: Core implementation

### Task 1: Scaffold header and build target

**Files:**

- Create: `z3wire/bitfield.h`

- Create: `z3wire/bitfield_test.cc`

- Modify: `z3wire/BUILD.bazel`

- [ ] **Step 1: Create the header with an include guard and empty namespace**

```cpp
// z3wire/bitfield.h
#ifndef Z3WIRE_BITFIELD_H_
#define Z3WIRE_BITFIELD_H_

#include "z3wire/bitvec.h"

namespace z3w {

// bitfield_eq will go here.

}  // namespace z3w

#endif  // Z3WIRE_BITFIELD_H_
```

- [ ] **Step 2: Create an empty test file**

```cpp
// z3wire/bitfield_test.cc
#include "z3wire/bitfield.h"

#include <gtest/gtest.h>

namespace z3w {
namespace {

class BitFieldTest : public ::testing::Test {
 protected:
  z3::context ctx_;
};

}  // namespace
}  // namespace z3w
```

- [ ] **Step 3: Add build targets to `z3wire/BUILD.bazel`**

Add after the existing `int_test` target:

```python
cc_library(
    name = "bitfield",
    hdrs = ["bitfield.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":bitvec",
    ],
)

cc_test(
    name = "bitfield_test",
    srcs = ["bitfield_test.cc"],
    deps = [
        ":bitfield",
        "@googletest//:gtest_main",
    ],
)
```

- [ ] **Step 4: Build and run the empty test**

Run: `./dev.sh bazel test //z3wire:bitfield_test`
Expected: PASS (0 tests run, but compiles and links)

- [ ] **Step 5: Format and commit**

```bash
./dev.sh ./tools/format.sh
git add z3wire/bitfield.h z3wire/bitfield_test.cc z3wire/BUILD.bazel
git commit -m "Scaffold bitfield_eq header and test"
```

______________________________________________________________________

### Task 2: Implement bitfield_eq for Ubv-only fields

**Files:**

- Modify: `z3wire/bitfield_test.cc`

- Modify: `z3wire/bitfield.h`

- [ ] **Step 1: Write failing test — two Ubv fields**

Add to `bitfield_test.cc`:

```cpp
TEST_F(BitFieldTest, TwoUbvFields) {
  Ubv<8> buf(ctx_, "buf");
  Ubv<3> lo(ctx_, "lo");
  Ubv<5> hi(ctx_, "hi");

  auto constraint = bitfield_eq(buf, lo, hi);

  // Verify return type is Bool.
  static_assert(std::is_same_v<decltype(constraint), Bool>);

  // If lo=0b101 and hi=0b11010, buf should equal 0b11010_101 = 0xD5.
  z3::solver s(ctx_);
  s.add(constraint.raw());
  s.add(lo.raw() == ctx_.bv_val(0b101, 3));
  s.add(hi.raw() == ctx_.bv_val(0b11010, 5));
  s.add(buf.raw() != ctx_.bv_val(0xD5, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `./dev.sh bazel test //z3wire:bitfield_test`
Expected: FAIL (bitfield_eq not defined)

- [ ] **Step 3: Implement bitfield_eq**

In `z3wire/bitfield.h`, replace the placeholder comment with:

```cpp
namespace internal {

// Width contribution of each field type.
template <typename T>
constexpr size_t field_width() {
  return T::kWidth;
}

// Convert a field to Ubv for concatenation. Returns by value.
template <size_t W, bool S>
Ubv<W> to_ubv_field(const BitVec<W, S>& field) {
  return cast<Ubv<W>>(field);
}

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
```

- [ ] **Step 4: Run test to verify it passes**

Run: `./dev.sh bazel test //z3wire:bitfield_test`
Expected: PASS

- [ ] **Step 5: Write test — single field (degenerate case)**

Add to `bitfield_test.cc`:

```cpp
TEST_F(BitFieldTest, SingleField) {
  Ubv<8> buf(ctx_, "buf");
  Ubv<8> field(ctx_, "field");

  auto constraint = bitfield_eq(buf, field);

  z3::solver s(ctx_);
  s.add(constraint.raw());
  s.add(field.raw() == ctx_.bv_val(42, 8));
  s.add(buf.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}
```

- [ ] **Step 6: Run test**

Run: `./dev.sh bazel test //z3wire:bitfield_test`
Expected: PASS

- [ ] **Step 7: Write test — three Ubv fields, verify bidirectional constraint**

Add to `bitfield_test.cc`:

```cpp
TEST_F(BitFieldTest, ThreeUbvFieldsBidirectional) {
  Ubv<8> buf(ctx_, "buf");
  Ubv<2> a(ctx_, "a");
  Ubv<3> b(ctx_, "b");
  Ubv<3> c(ctx_, "c");

  z3::solver s(ctx_);
  s.add(bitfield_eq(buf, a, b, c).raw());

  // Constrain buf, verify fields are determined.
  // buf = 0b110_010_01 = 0xC9
  // LSB-first: a = bits[1:0] = 0b01, b = bits[4:2] = 0b010, c = bits[7:5] = 0b110
  s.add(buf.raw() == ctx_.bv_val(0xC9, 8));

  s.add((a.raw() != ctx_.bv_val(0b01, 2)) ||
        (b.raw() != ctx_.bv_val(0b010, 3)) ||
        (c.raw() != ctx_.bv_val(0b110, 3)));
  EXPECT_EQ(s.check(), z3::unsat);
}
```

- [ ] **Step 8: Run test**

Run: `./dev.sh bazel test //z3wire:bitfield_test`
Expected: PASS

- [ ] **Step 9: Format and commit**

```bash
./dev.sh ./tools/format.sh
git add z3wire/bitfield.h z3wire/bitfield_test.cc
git commit -m "Implement bitfield_eq for Ubv fields"
```

______________________________________________________________________

### Task 3: Add Bool field support

**Files:**

- Modify: `z3wire/bitfield_test.cc`

- Modify: `z3wire/bitfield.h`

- [ ] **Step 1: Write failing test — Bool field**

Add to `bitfield_test.cc`:

```cpp
TEST_F(BitFieldTest, BoolField) {
  Ubv<4> buf(ctx_, "buf");
  Bool flag(ctx_, "flag");
  Ubv<3> data(ctx_, "data");

  z3::solver s(ctx_);
  s.add(bitfield_eq(buf, flag, data).raw());

  // flag=true (bit 0 = 1), data=0b101 (bits 3..1)
  // buf = 0b101_1 = 0xB
  s.add(flag.raw() == ctx_.bool_val(true));
  s.add(data.raw() == ctx_.bv_val(0b101, 3));
  s.add(buf.raw() != ctx_.bv_val(0xB, 4));
  EXPECT_EQ(s.check(), z3::unsat);
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `./dev.sh bazel test //z3wire:bitfield_test`
Expected: FAIL (`field_width` and `to_ubv_field` don't handle `Bool`)

- [ ] **Step 3: Add Bool specializations**

In `z3wire/bitfield.h`, add Bool specializations to the `internal` namespace:

```cpp
// Bool has width 1.
template <>
constexpr size_t field_width<Bool>() {
  return 1;
}

// Bool -> Ubv<1> conversion.
inline Ubv<1> to_ubv_field(const Bool& field) { return to_ubv1(field); }
```

- [ ] **Step 4: Run test to verify it passes**

Run: `./dev.sh bazel test //z3wire:bitfield_test`
Expected: PASS

- [ ] **Step 5: Format and commit**

```bash
./dev.sh ./tools/format.sh
git add z3wire/bitfield.h z3wire/bitfield_test.cc
git commit -m "Support Bool fields in bitfield_eq"
```

______________________________________________________________________

### Task 4: Add Sbv field test

**Files:**

- Modify: `z3wire/bitfield_test.cc`

Sbv is already handled by `to_ubv_field` via `cast<Ubv<W>>()` (same-width cast
is a zero-cost bitcast — see `bitvec.h:378-380`). This task just adds a test
to confirm.

- [ ] **Step 1: Write test — Sbv field**

Add to `bitfield_test.cc`:

```cpp
TEST_F(BitFieldTest, SbvField) {
  Ubv<8> buf(ctx_, "buf");
  Ubv<3> lo(ctx_, "lo");
  Sbv<5> hi(ctx_, "hi");

  z3::solver s(ctx_);
  s.add(bitfield_eq(buf, lo, hi).raw());

  // lo=0b010 (bits 2..0), hi=-1 as Sbv<5> = 0b11111 (bits 7..3)
  // buf = 0b11111_010 = 0xFA
  s.add(lo.raw() == ctx_.bv_val(0b010, 3));
  s.add(hi.raw() == ctx_.bv_val(0b11111, 5));
  s.add(buf.raw() != ctx_.bv_val(0xFA, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}
```

- [ ] **Step 2: Run test to verify it passes**

Run: `./dev.sh bazel test //z3wire:bitfield_test`
Expected: PASS

- [ ] **Step 3: Format and commit**

```bash
./dev.sh ./tools/format.sh
git add z3wire/bitfield_test.cc
git commit -m "Add Sbv field test for bitfield_eq"
```

______________________________________________________________________

### Task 5: Mixed field types test

**Files:**

- Modify: `z3wire/bitfield_test.cc`

- [ ] **Step 1: Write test — Bool + Ubv + Sbv (the motivating example)**

Add to `bitfield_test.cc`:

```cpp
TEST_F(BitFieldTest, MixedFieldTypes) {
  // The motivating example from the design doc.
  Ubv<8> md(ctx_, "md");
  Bool x(ctx_, "x");       // bit 0
  Ubv<3> y(ctx_, "y");     // bits 3..1
  Sbv<4> z(ctx_, "z");     // bits 7..4

  z3::solver s(ctx_);
  s.add(bitfield_eq(md, x, y, z).raw());

  // x=true(1), y=0b110, z=0b1010 (signed -6)
  // md = 0b1010_110_1 = 0xAD
  s.add(x.raw() == ctx_.bool_val(true));
  s.add(y.raw() == ctx_.bv_val(0b110, 3));
  s.add(z.raw() == ctx_.bv_val(0b1010, 4));
  s.add(md.raw() != ctx_.bv_val(0xAD, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}
```

- [ ] **Step 2: Run test**

Run: `./dev.sh bazel test //z3wire:bitfield_test`
Expected: PASS

- [ ] **Step 3: Format and commit**

```bash
./dev.sh ./tools/format.sh
git add z3wire/bitfield_test.cc
git commit -m "Add mixed-type bitfield_eq test (Bool + Ubv + Sbv)"
```

______________________________________________________________________

### Task 6: Compile-fail test for width mismatch

**Files:**

- Create: `compile_fail_tests/bitfield_eq_width_mismatch_test.cc`

- Modify: `compile_fail_tests/BUILD.bazel`

- [ ] **Step 1: Create the compile-fail test source**

```cpp
// compile_fail_tests/bitfield_eq_width_mismatch_test.cc
#include "z3wire/bitfield.h"

// Field widths (3 + 3 = 6) do not sum to buffer width (8).
void trigger() {
  z3::context ctx;
  z3w::Ubv<8> buf(ctx, "buf");
  z3w::Ubv<3> a(ctx, "a");
  z3w::Ubv<3> b(ctx, "b");
  auto eq = z3w::bitfield_eq(buf, a, b);
}
```

- [ ] **Step 2: Add the build target to `compile_fail_tests/BUILD.bazel`**

```python
cc_compile_fail_test(
    name = "bitfield_eq_width_mismatch_test",
    src = "bitfield_eq_width_mismatch_test.cc",
    expected_message = "Field widths must sum to the buffer width",
    deps = ["//z3wire:bitfield"],
)
```

- [ ] **Step 3: Run the compile-fail test**

Run: `./dev.sh bazel test //compile_fail_tests:bitfield_eq_width_mismatch_test`
Expected: PASS (compilation fails with expected message)

- [ ] **Step 4: Format and commit**

```bash
./dev.sh ./tools/format.sh
git add compile_fail_tests/bitfield_eq_width_mismatch_test.cc compile_fail_tests/BUILD.bazel
git commit -m "Add compile-fail test for bitfield_eq width mismatch"
```

______________________________________________________________________

## Chunk 2: Documentation

### Task 7: Update operations doc

**Files:**

- Modify: `docs/usage/operations.md`

- [ ] **Step 1: Add "Bit field" section before the "Mux" section**

Add before the `## Mux` heading in `docs/usage/operations.md`. The section
should contain:

- An intro explaining what `bitfield_eq` returns

- A basic usage example showing `Bool`, `Ubv`, and `Sbv` fields with an 8-bit
    buffer

- An explanation that fields are mapped LSB-first, matching Amaranth and Chisel
    convention, and that this is the opposite of `concat`

- An admonition (`!!! note`) clarifying the LSB-first vs MSB-first difference
    with an equivalent manual `concat` expression

- A list of how each field type is handled (Bool via `to_ubv1`, Sbv
    reinterpreted, Ubv as-is)

- A note that `static_assert` verifies width sum

- A note that Z3 handles bidirectional reasoning automatically

- A multi-level decomposition example showing chained `bitfield_eq` calls

- [ ] **Step 2: Run docs build**

Run: `./dev.sh ./tools/docs.sh build`
Expected: Builds without warnings

- [ ] **Step 3: Format and commit**

```bash
./dev.sh ./tools/format.sh
git add docs/usage/operations.md
git commit -m "Document bitfield_eq in operations guide"
```

______________________________________________________________________

### Task 8: Update cheatsheet

**Files:**

- Modify: `docs/usage/cheatsheet.md`

- [ ] **Step 1: Add a "Bit field" section after "Bit manipulation"**

Add a new section after the "Bit manipulation" section (line 89) and before the
"Casting" section (line 91). The section uses a code block to match the style
of the "Bit manipulation" section:

````markdown
## Bit field

```cpp
bitfield_eq(buf, f1, f2, ...)  // -> Bool (LSB-first field equality constraint)
````

See [Operations](operations.md#bit-field).

````

- [ ] **Step 2: Run docs build**

Run: `./dev.sh ./tools/docs.sh build`
Expected: Builds without warnings

- [ ] **Step 3: Format and commit**

```bash
./dev.sh ./tools/format.sh
git add docs/usage/cheatsheet.md
git commit -m "Add bitfield_eq to cheatsheet"
````

______________________________________________________________________

### Task 9: Final verification

- [ ] **Step 1: Run formatter**

Run: `./dev.sh ./tools/format.sh`

- [ ] **Step 2: Run all tests**

Run: `./dev.sh bazel test //...`
Expected: All tests pass (27 existing + new bitfield tests + new compile-fail
test)

- [ ] **Step 3: Commit formatting changes if any**

```bash
./dev.sh ./tools/format.sh --check
```

If any files need formatting, format and commit:

```bash
./dev.sh ./tools/format.sh
git add z3wire/bitfield.h z3wire/bitfield_test.cc z3wire/BUILD.bazel docs/usage/operations.md docs/usage/cheatsheet.md
git commit -m "Format bitfield_eq files"
```
