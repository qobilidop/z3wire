#include "z3wire/weave/emit_proto.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "z3wire/weave/rdl.pb.h"
#include "z3wire/weave/resolver.h"

namespace z3wire::weave {
namespace {

using ::testing::HasSubstr;
using ::testing::Not;

// Helper to create a module with common defaults.
z3wire_rdl::Module MakeModule(const std::string& ns = "test") {
  z3wire_rdl::Module m;
  m.set_file_prefix("test");
  m.set_namespace_(ns);
  m.set_field_pack_order(z3wire_rdl::FIELD_PACK_ORDER_LSB_FIRST);
  return m;
}

TEST(EmitProtoTest, SimpleStruct) {
  auto module = MakeModule();

  auto* s = module.add_structs();
  s->set_name("Simple");
  s->set_desc("A simple struct");

  {
    auto* f = s->add_fields();
    f->set_name("flag");
    f->mutable_type()->mutable_bool_();
  }
  {
    auto* f = s->add_fields();
    f->set_name("value");
    f->mutable_type()->mutable_bitvec()->set_width(8);
  }
  {
    auto* f = s->add_fields();
    f->set_name("pad");
    f->mutable_type()->mutable_bitvec()->set_width(7);
    f->set_reserved(true);
  }

  auto resolved = Resolve(module);
  std::string output = EmitProto(resolved);

  EXPECT_THAT(output, HasSubstr("syntax = \"proto3\";"));
  EXPECT_THAT(output, HasSubstr("package test;"));
  EXPECT_THAT(output, HasSubstr("message SimpleProto"));
  EXPECT_THAT(output, HasSubstr("bool flag = 1;"));
  EXPECT_THAT(output, HasSubstr("uint32 value = 2;"));
  // Reserved field should be omitted.
  EXPECT_THAT(output, Not(HasSubstr("pad")));
}

TEST(EmitProtoTest, NestedAndSigned) {
  auto module = MakeModule("example");

  {
    auto* s = module.add_structs();
    s->set_name("Inner");
    auto* f = s->add_fields();
    f->set_name("val");
    auto* bv = f->mutable_type()->mutable_bitvec();
    bv->set_width(4);
    bv->set_signedness(z3wire_rdl::SIGNEDNESS_SIGNED);
  }
  {
    auto* s = module.add_structs();
    s->set_name("Outer");
    {
      auto* f = s->add_fields();
      f->set_name("inner");
      f->mutable_type()->set_struct_ref("Inner");
    }
    {
      auto* f = s->add_fields();
      f->set_name("data");
      f->mutable_type()->mutable_bitvec()->set_width(4);
      f->set_count(3);
    }
  }

  auto resolved = Resolve(module);
  std::string output = EmitProto(resolved);

  EXPECT_THAT(output, HasSubstr("InnerProto"));
  EXPECT_THAT(output, HasSubstr("sint32 val = 1;"));
  EXPECT_THAT(output, HasSubstr("InnerProto inner = 1;"));
  EXPECT_THAT(output, HasSubstr("repeated uint32 data = 2;"));
}

TEST(EmitProtoTest, EnumRefField) {
  auto module = MakeModule();

  {
    auto* e = module.add_enums();
    e->set_name("Mode");
    e->set_width(2);
    {
      auto* v = e->add_values();
      v->set_name("kA");
      v->set_value(0);
    }
    {
      auto* v = e->add_values();
      v->set_name("kB");
      v->set_value(1);
    }
  }
  {
    auto* s = module.add_structs();
    s->set_name("S");
    auto* f = s->add_fields();
    f->set_name("mode");
    f->mutable_type()->set_enum_ref("Mode");
  }

  auto resolved = Resolve(module);
  std::string output = EmitProto(resolved);

  EXPECT_THAT(output, HasSubstr("uint32 mode = 1;"));
}

}  // namespace
}  // namespace z3wire::weave
