#include "z3wire/bitfield.h"

#include <type_traits>

#include <gtest/gtest.h>

namespace z3w {
namespace {

class BitFieldTest : public ::testing::Test {
 protected:
  z3::context ctx_;
};

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

TEST_F(BitFieldTest, ThreeUbvFieldsBidirectional) {
  Ubv<8> buf(ctx_, "buf");
  Ubv<2> a(ctx_, "a");
  Ubv<3> b(ctx_, "b");
  Ubv<3> c(ctx_, "c");

  z3::solver s(ctx_);
  s.add(bitfield_eq(buf, a, b, c).raw());

  // Constrain buf, verify fields are determined.
  // buf = 0b110_010_01 = 0xC9
  // LSB-first: a = bits[1:0] = 0b01, b = bits[4:2] = 0b010, c = bits[7:5] =
  // 0b110
  s.add(buf.raw() == ctx_.bv_val(0xC9, 8));

  s.add((a.raw() != ctx_.bv_val(0b01, 2)) ||
        (b.raw() != ctx_.bv_val(0b010, 3)) ||
        (c.raw() != ctx_.bv_val(0b110, 3)));
  EXPECT_EQ(s.check(), z3::unsat);
}

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

}  // namespace
}  // namespace z3w
