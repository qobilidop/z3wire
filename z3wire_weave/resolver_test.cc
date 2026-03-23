#include "z3wire_weave/resolver.h"

#include <stdexcept>

#include "gtest/gtest.h"
#include "z3wire_weave/wire_spec.pb.h"

namespace z3wire_weave {
namespace {

// Helper to create a module with common defaults.
z3wire_weave::WireSpec MakeModule() {
  z3wire_weave::WireSpec m;
  m.set_file_prefix("test");
  m.set_namespace_("test");
  m.set_field_pack_order(z3wire_weave::FIELD_PACK_ORDER_LSB_FIRST);
  return m;
}

TEST(ResolverTest, BoolFieldWidth) {
  auto module = MakeModule();
  auto* s = module.add_structs();
  s->set_name("Simple");
  auto* f = s->add_fields();
  f->set_name("flag");
  f->mutable_type()->mutable_bool_();

  auto resolved = Resolve(module);
  EXPECT_EQ(resolved.structs[0].total_width, 1);
  EXPECT_EQ(resolved.structs[0].fields[0].width, 1);
  EXPECT_EQ(resolved.structs[0].fields[0].offset, 0);
}

TEST(ResolverTest, BitVecFieldWidth) {
  auto module = MakeModule();
  auto* s = module.add_structs();
  s->set_name("S");
  auto* f1 = s->add_fields();
  f1->set_name("a");
  f1->mutable_type()->mutable_bitvec()->set_width(8);
  auto* f2 = s->add_fields();
  f2->set_name("b");
  f2->mutable_type()->mutable_bitvec()->set_width(4);

  auto resolved = Resolve(module);
  EXPECT_EQ(resolved.structs[0].total_width, 12);
  EXPECT_EQ(resolved.structs[0].fields[0].offset, 0);
  EXPECT_EQ(resolved.structs[0].fields[1].offset, 8);
}

TEST(ResolverTest, CountZeroIsScalarCountOneIsArray) {
  auto module = MakeModule();
  auto* s = module.add_structs();
  s->set_name("S");
  auto* f1 = s->add_fields();
  f1->set_name("scalar");
  f1->mutable_type()->mutable_bitvec()->set_width(8);
  f1->set_count(0);
  auto* f2 = s->add_fields();
  f2->set_name("array");
  f2->mutable_type()->mutable_bitvec()->set_width(8);
  f2->set_count(1);

  auto resolved = Resolve(module);
  EXPECT_EQ(resolved.structs[0].fields[0].count, 0);
  EXPECT_EQ(resolved.structs[0].fields[0].width, 8);
  EXPECT_EQ(resolved.structs[0].fields[1].count, 1);
  EXPECT_EQ(resolved.structs[0].fields[1].width, 8);
}

TEST(ResolverTest, UnresolvedEnumRef) {
  auto module = MakeModule();
  auto* s = module.add_structs();
  s->set_name("S");
  auto* f = s->add_fields();
  f->set_name("x");
  f->mutable_type()->set_enum_ref("NoSuchEnum");

  EXPECT_THROW(Resolve(module), std::invalid_argument);
}

TEST(ResolverTest, UnresolvedStructRef) {
  auto module = MakeModule();
  auto* s = module.add_structs();
  s->set_name("S");
  auto* f = s->add_fields();
  f->set_name("x");
  f->mutable_type()->set_struct_ref("NoSuchStruct");

  EXPECT_THROW(Resolve(module), std::invalid_argument);
}

TEST(ResolverTest, EnumValueExceedsWidth) {
  auto module = MakeModule();
  auto* e = module.add_enums();
  e->set_name("E");
  e->set_width(2);
  auto* v = e->add_values();
  v->set_name("kBig");
  v->set_value(4);

  EXPECT_THROW(Resolve(module), std::invalid_argument);
}

TEST(ResolverTest, DuplicateEnumValues) {
  auto module = MakeModule();
  auto* e = module.add_enums();
  e->set_name("E");
  e->set_width(2);
  auto* v1 = e->add_values();
  v1->set_name("kA");
  v1->set_value(0);
  auto* v2 = e->add_values();
  v2->set_name("kB");
  v2->set_value(0);

  EXPECT_THROW(Resolve(module), std::invalid_argument);
}

TEST(ResolverTest, WidthMismatch) {
  auto module = MakeModule();
  auto* s = module.add_structs();
  s->set_name("S");
  s->set_width(16);
  auto* f = s->add_fields();
  f->set_name("x");
  f->mutable_type()->mutable_bool_();

  EXPECT_THROW(Resolve(module), std::invalid_argument);
}

TEST(ResolverTest, MissingFieldPackOrder) {
  z3wire_weave::WireSpec module;
  module.set_file_prefix("test");
  module.set_namespace_("test");
  // No field_pack_order set.
  auto* s = module.add_structs();
  s->set_name("S");
  auto* f = s->add_fields();
  f->set_name("x");
  f->mutable_type()->mutable_bool_();

  EXPECT_THROW(Resolve(module), std::invalid_argument);
}

TEST(ResolverTest, MissingFilePrefix) {
  z3wire_weave::WireSpec module;
  module.set_namespace_("test");
  module.set_field_pack_order(z3wire_weave::FIELD_PACK_ORDER_LSB_FIRST);

  EXPECT_THROW(Resolve(module), std::invalid_argument);
}

TEST(ResolverTest, CircularStructDependency) {
  auto module = MakeModule();
  auto* s1 = module.add_structs();
  s1->set_name("A");
  auto* f1 = s1->add_fields();
  f1->set_name("b");
  f1->mutable_type()->set_struct_ref("B");

  auto* s2 = module.add_structs();
  s2->set_name("B");
  auto* f2 = s2->add_fields();
  f2->set_name("a");
  f2->mutable_type()->set_struct_ref("A");

  EXPECT_THROW(Resolve(module), std::invalid_argument);
}

TEST(ResolverTest, MsbFirstOffsets) {
  auto module = MakeModule();
  module.set_field_pack_order(z3wire_weave::FIELD_PACK_ORDER_MSB_FIRST);
  auto* s = module.add_structs();
  s->set_name("S");
  s->set_width(8);

  auto* f1 = s->add_fields();
  f1->set_name("high");
  f1->mutable_type()->mutable_bitvec()->set_width(3);

  auto* f2 = s->add_fields();
  f2->set_name("mid");
  f2->mutable_type()->mutable_bitvec()->set_width(4);

  auto* f3 = s->add_fields();
  f3->set_name("low");
  f3->mutable_type()->mutable_bool_();

  auto resolved = Resolve(module);
  const auto& rs = resolved.structs[0];
  EXPECT_EQ(rs.total_width, 8);
  EXPECT_EQ(rs.field_pack_order, ResolvedStruct::kMsbFirst);
  EXPECT_EQ(rs.fields[0].name, "high");
  EXPECT_EQ(rs.fields[0].offset, 5);
  EXPECT_EQ(rs.fields[0].width, 3);
  EXPECT_EQ(rs.fields[1].name, "mid");
  EXPECT_EQ(rs.fields[1].offset, 1);
  EXPECT_EQ(rs.fields[1].width, 4);
  EXPECT_EQ(rs.fields[2].name, "low");
  EXPECT_EQ(rs.fields[2].offset, 0);
  EXPECT_EQ(rs.fields[2].width, 1);
}

TEST(ResolverTest, ReservedFieldNameCollision) {
  auto module = MakeModule();
  auto* s = module.add_structs();
  s->set_name("S");
  auto* f = s->add_fields();
  f->set_name("Pack");
  f->mutable_type()->mutable_bool_();

  EXPECT_THROW(Resolve(module), std::invalid_argument);
}

TEST(ResolverTest, FullExample) {
  z3wire_weave::WireSpec module;
  module.set_file_prefix("status_register");
  module.set_namespace_("example");
  module.set_field_pack_order(z3wire_weave::FIELD_PACK_ORDER_LSB_FIRST);

  // Enum: OpMode
  auto* e = module.add_enums();
  e->set_name("OpMode");
  e->set_desc("Operating mode");
  e->set_width(2);
  auto* v1 = e->add_values();
  v1->set_name("kIdle");
  v1->set_value(0);
  auto* v2 = e->add_values();
  v2->set_name("kActive");
  v2->set_value(1);
  auto* v3 = e->add_values();
  v3->set_name("kSleep");
  v3->set_value(2);

  // Struct: ErrorInfo (8 bits)
  auto* ei = module.add_structs();
  ei->set_name("ErrorInfo");
  ei->set_desc("Error information");
  ei->set_width(8);
  {
    auto* f = ei->add_fields();
    f->set_name("code");
    f->mutable_type()->mutable_bitvec()->set_width(4);
  }
  {
    auto* f = ei->add_fields();
    f->set_name("severity");
    auto* bv = f->mutable_type()->mutable_bitvec();
    bv->set_width(2);
    bv->set_signedness(z3wire_weave::SIGNEDNESS_SIGNED);
  }
  {
    auto* f = ei->add_fields();
    f->set_name("fatal");
    f->mutable_type()->mutable_bool_();
  }
  {
    auto* f = ei->add_fields();
    f->set_name("reserved");
    f->mutable_type()->mutable_bitvec()->set_width(1);
    f->set_reserved(true);
  }

  // Struct: StatusRegister (32 bits)
  auto* sr = module.add_structs();
  sr->set_name("StatusRegister");
  sr->set_desc("Device status register");
  sr->set_width(32);
  {
    auto* f = sr->add_fields();
    f->set_name("ready");
    f->mutable_type()->mutable_bool_();
  }
  {
    auto* f = sr->add_fields();
    f->set_name("mode");
    f->mutable_type()->set_enum_ref("OpMode");
  }
  {
    auto* f = sr->add_fields();
    f->set_name("error");
    f->mutable_type()->set_struct_ref("ErrorInfo");
  }
  {
    auto* f = sr->add_fields();
    f->set_name("counters");
    f->mutable_type()->mutable_bitvec()->set_width(4);
    f->set_count(4);
  }
  {
    auto* f = sr->add_fields();
    f->set_name("reserved");
    f->mutable_type()->mutable_bitvec()->set_width(5);
    f->set_reserved(true);
  }

  auto resolved = Resolve(module);
  EXPECT_EQ(resolved.ns, "example");

  // Enum
  ASSERT_EQ(resolved.enums.size(), 1);
  EXPECT_EQ(resolved.enums[0].name, "OpMode");
  EXPECT_EQ(resolved.enums[0].width, 2);

  // ErrorInfo: 4 + 2 + 1 + 1 = 8
  ASSERT_GE(resolved.structs.size(), 1);
  const auto& ei_r = resolved.structs[0];
  EXPECT_EQ(ei_r.name, "ErrorInfo");
  EXPECT_EQ(ei_r.total_width, 8);
  EXPECT_EQ(ei_r.fields[0].offset, 0);  // code [3:0]
  EXPECT_EQ(ei_r.fields[1].offset, 4);  // severity [5:4]
  EXPECT_EQ(ei_r.fields[2].offset, 6);  // fatal [6]
  EXPECT_EQ(ei_r.fields[3].offset, 7);  // reserved [7]

  // StatusRegister: 1 + 2 + 8 + 16 + 5 = 32
  ASSERT_GE(resolved.structs.size(), 2);
  const auto& sr_r = resolved.structs[1];
  EXPECT_EQ(sr_r.name, "StatusRegister");
  EXPECT_EQ(sr_r.total_width, 32);
  EXPECT_EQ(sr_r.fields[0].offset, 0);   // ready [0]
  EXPECT_EQ(sr_r.fields[1].offset, 1);   // mode [2:1]
  EXPECT_EQ(sr_r.fields[2].offset, 3);   // error [10:3]
  EXPECT_EQ(sr_r.fields[3].offset, 11);  // counters [26:11]
  EXPECT_EQ(sr_r.fields[4].offset, 27);  // reserved [31:27]

  // Counters: array of 4 x 4-bit
  EXPECT_EQ(sr_r.fields[3].count, 4);
  EXPECT_EQ(sr_r.fields[3].element_width, 4);
  EXPECT_EQ(sr_r.fields[3].width, 16);
}

}  // namespace
}  // namespace z3wire_weave
