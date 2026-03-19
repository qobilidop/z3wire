#include "examples/weave/status_register.h"

#include <gtest/gtest.h>

namespace example {
namespace {

TEST(StatusRegisterTest, EnumConstants) {
  EXPECT_EQ(OpMode::kIdle.value(), 0);
  EXPECT_EQ(OpMode::kActive.value(), 1);
  EXPECT_EQ(OpMode::kSleep.value(), 2);
}

TEST(StatusRegisterTest, CreateSymbolicVariables) {
  z3::context ctx;
  auto sr = StatusRegisterSymbolic::Create(ctx, "sr");

  // Verify variables were created (non-null expressions)
  EXPECT_NE(sr.ready.expr().to_string(), "");
  EXPECT_NE(sr.mode.expr().to_string(), "");
  EXPECT_NE(sr.error.code.expr().to_string(), "");
  EXPECT_NE(sr.counters[0].expr().to_string(), "");
  EXPECT_NE(sr.counters[3].expr().to_string(), "");
}

TEST(StatusRegisterTest, PackWidth) {
  z3::context ctx;
  auto sr = StatusRegisterSymbolic::Create(ctx, "sr");
  auto packed = sr.Pack();

  // Pack should produce a 32-bit vector
  EXPECT_EQ(packed.expr().get_sort().bv_size(), 32u);
}

TEST(StatusRegisterTest, NestedPackWidth) {
  z3::context ctx;
  auto ei = ErrorInfoSymbolic::Create(ctx, "ei");
  auto packed = ei.Pack();

  // ErrorInfo is 8 bits
  EXPECT_EQ(packed.expr().get_sort().bv_size(), 8u);
}

TEST(StatusRegisterTest, RoundtripConcreteSymbolicConcrete) {
  z3::context ctx;

  // Create a concrete value
  ErrorInfoConcrete ei_orig{};
  ei_orig.code = std::get<0>(z3w::UInt<4>::Checked(5));
  ei_orig.severity = std::get<0>(z3w::SInt<2>::Checked(-1));
  ei_orig.fatal = true;
  ei_orig.reserved = std::get<0>(z3w::UInt<1>::Checked(0));

  // Convert to symbolic
  auto ei_sym = ErrorInfoSymbolic::FromConcrete(ctx, ei_orig);

  // Evaluate back to concrete (using a solver to get a model)
  z3::solver solver(ctx);
  solver.add(ei_sym.code.expr() == ctx.bv_val(5, 4));
  solver.add(ei_sym.severity.expr() ==
             ctx.bv_val(static_cast<uint64_t>(-1LL) & 0x3, 2));
  solver.add(ei_sym.fatal.expr() == ctx.bool_val(true));
  solver.add(ei_sym.reserved.expr() == ctx.bv_val(0, 1));
  ASSERT_EQ(solver.check(), z3::sat);
  auto model = solver.get_model();

  auto ei_back = ei_sym.ToConcrete(model);

  EXPECT_EQ(ei_back.code.value(), 5);
  EXPECT_EQ(ei_back.severity.value(), -1);
  EXPECT_EQ(ei_back.fatal, true);
}

TEST(StatusRegisterTest, PackConstraint) {
  z3::context ctx;
  z3::solver solver(ctx);

  auto sr = StatusRegisterSymbolic::Create(ctx, "sr");

  // Constrain the packed value to a known bit pattern
  // ready=1 (bit 0), mode=2 (bits 2:1), rest=0
  // Binary: ...000 | 10 | 1 = 0x00000005
  auto packed = sr.Pack();
  solver.add(packed.expr() == ctx.bv_val(5, 32));

  ASSERT_EQ(solver.check(), z3::sat);
  auto model = solver.get_model();

  auto concrete = sr.ToConcrete(model);
  EXPECT_EQ(concrete.ready, true);      // bit 0 = 1
  EXPECT_EQ(concrete.mode.value(), 2);  // bits 2:1 = 10
}

TEST(StatusRegisterTest, ConcreteToProtoRoundtrip) {
  ErrorInfoConcrete ei{};
  ei.code = std::get<0>(z3w::UInt<4>::Checked(7));
  ei.severity = std::get<0>(z3w::SInt<2>::Checked(-2));
  ei.fatal = false;
  ei.reserved = std::get<0>(z3w::UInt<1>::Checked(0));

  StatusRegisterConcrete sr{};
  sr.ready = true;
  sr.mode = std::get<0>(z3w::UInt<2>::Checked(1));
  sr.error = ei;
  sr.counters[0] = std::get<0>(z3w::UInt<4>::Checked(1));
  sr.counters[1] = std::get<0>(z3w::UInt<4>::Checked(2));
  sr.counters[2] = std::get<0>(z3w::UInt<4>::Checked(3));
  sr.counters[3] = std::get<0>(z3w::UInt<4>::Checked(4));
  sr.reserved = std::get<0>(z3w::UInt<5>::Checked(0));

  // Convert to proto and back
  auto proto = sr.ToProto();
  auto sr_back = StatusRegisterConcrete::FromProto(proto);

  EXPECT_EQ(sr_back.ready, true);
  EXPECT_EQ(sr_back.mode.value(), 1);
  EXPECT_EQ(sr_back.error.code.value(), 7);
  EXPECT_EQ(sr_back.error.severity.value(), -2);
  EXPECT_EQ(sr_back.error.fatal, false);
  EXPECT_EQ(sr_back.counters[0].value(), 1);
  EXPECT_EQ(sr_back.counters[1].value(), 2);
  EXPECT_EQ(sr_back.counters[2].value(), 3);
  EXPECT_EQ(sr_back.counters[3].value(), 4);
}

TEST(StatusRegisterTest, ProtoOmitsReservedFields) {
  StatusRegisterConcrete sr{};
  sr.ready = false;
  sr.mode = std::get<0>(z3w::UInt<2>::Checked(0));
  sr.counters[0] = std::get<0>(z3w::UInt<4>::Checked(0));
  sr.counters[1] = std::get<0>(z3w::UInt<4>::Checked(0));
  sr.counters[2] = std::get<0>(z3w::UInt<4>::Checked(0));
  sr.counters[3] = std::get<0>(z3w::UInt<4>::Checked(0));
  sr.reserved = std::get<0>(z3w::UInt<5>::Checked(31));

  auto proto = sr.ToProto();

  // Reserved fields should not be in the proto
  // The proto message doesn't have a 'reserved' field, so it can't store it.
  // After roundtrip, reserved should be zero-initialized (default).
  auto sr_back = StatusRegisterConcrete::FromProto(proto);
  EXPECT_EQ(sr_back.reserved.value(), 0);  // Not preserved through proto
}

}  // namespace
}  // namespace example
