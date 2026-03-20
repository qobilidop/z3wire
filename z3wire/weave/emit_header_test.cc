#include "z3wire/weave/emit_header.h"

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

TEST(EmitHeaderTest, EnumConstants) {
  auto module = MakeModule();

  auto* e = module.add_enums();
  e->set_name("OpMode");
  e->set_desc("Operating mode");
  e->set_width(2);
  {
    auto* v = e->add_values();
    v->set_name("kIdle");
    v->set_value(0);
  }
  {
    auto* v = e->add_values();
    v->set_name("kActive");
    v->set_value(1);
  }

  auto resolved = Resolve(module);
  std::string output = EmitHeader(resolved, "test.pb.h");

  EXPECT_THAT(output, HasSubstr("#pragma once"));
  EXPECT_THAT(output, HasSubstr("namespace test {"));
  EXPECT_THAT(output, HasSubstr("struct OpMode {"));
  EXPECT_THAT(output,
              HasSubstr("static constexpr auto kIdle = "
                        "z3w::UInt<2>::Literal<0>();"));
  EXPECT_THAT(output,
              HasSubstr("static constexpr auto kActive = "
                        "z3w::UInt<2>::Literal<1>();"));
  EXPECT_THAT(output, HasSubstr("}  // namespace test"));
}

TEST(EmitHeaderTest, ConcreteStruct) {
  auto module = MakeModule();

  auto* s = module.add_structs();
  s->set_name("Reg");
  s->set_desc("A register");
  {
    auto* f = s->add_fields();
    f->set_name("flag");
    f->mutable_type()->mutable_bool_();
  }
  {
    auto* f = s->add_fields();
    f->set_name("val");
    f->mutable_type()->mutable_bitvec()->set_width(4);
    f->mutable_type()->mutable_bitvec()->set_signedness(
        z3wire_rdl::SIGNEDNESS_SIGNED);
  }
  {
    auto* f = s->add_fields();
    f->set_name("items");
    f->mutable_type()->mutable_bitvec()->set_width(8);
    f->set_count(2);
  }

  auto resolved = Resolve(module);
  std::string output = EmitHeader(resolved, "test.pb.h");

  EXPECT_THAT(output, HasSubstr("struct RegConcrete {"));
  EXPECT_THAT(output, HasSubstr("z3w::Bool flag;"));
  EXPECT_THAT(output, HasSubstr("z3w::SInt<4> val;"));
  EXPECT_THAT(output, HasSubstr("std::array<z3w::UInt<8>, 2> items;"));
  EXPECT_THAT(output, HasSubstr("RegProto ToProto() const;"));
  EXPECT_THAT(output,
              HasSubstr("static RegConcrete FromProto("
                        "const RegProto& proto);"));
}

TEST(EmitHeaderTest, SymbolicStruct) {
  auto module = MakeModule();

  auto* s = module.add_structs();
  s->set_name("Reg");
  {
    auto* f = s->add_fields();
    f->set_name("flag");
    f->mutable_type()->mutable_bool_();
  }
  {
    auto* f = s->add_fields();
    f->set_name("val");
    f->mutable_type()->mutable_bitvec()->set_width(7);
  }

  auto resolved = Resolve(module);
  std::string output = EmitHeader(resolved, "test.pb.h");

  EXPECT_THAT(output, HasSubstr("struct RegSymbolic {"));
  EXPECT_THAT(output, HasSubstr("z3w::SymBool flag;"));
  EXPECT_THAT(output, HasSubstr("z3w::SymUInt<7> val;"));
  EXPECT_THAT(output, HasSubstr("// [0]"));
  EXPECT_THAT(output, HasSubstr("// [7:1]"));
  EXPECT_THAT(output, HasSubstr("z3w::SymUInt<8> Pack() const;"));
  EXPECT_THAT(output,
              HasSubstr("static RegSymbolic Create(z3::context& ctx,"));
}

TEST(EmitHeaderTest, ToProtoBody) {
  auto module = MakeModule();

  auto* s = module.add_structs();
  s->set_name("Reg");
  {
    auto* f = s->add_fields();
    f->set_name("flag");
    f->mutable_type()->mutable_bool_();
  }
  {
    auto* f = s->add_fields();
    f->set_name("val");
    f->mutable_type()->mutable_bitvec()->set_width(8);
  }
  {
    auto* f = s->add_fields();
    f->set_name("pad");
    f->mutable_type()->mutable_bitvec()->set_width(7);
    f->set_reserved(true);
  }

  auto resolved = Resolve(module);
  std::string output = EmitHeader(resolved, "test.pb.h");

  EXPECT_THAT(output,
              HasSubstr("inline RegProto RegConcrete::ToProto() const {"));
  EXPECT_THAT(output, HasSubstr("proto.set_flag(bool(flag));"));
  EXPECT_THAT(
      output,
      HasSubstr("proto.set_val(static_cast<uint32_t>(val.value()));"));
  // Reserved field should not appear in ToProto
  auto to_proto_pos = output.find("ToProto");
  auto brace_pos = output.find("}", to_proto_pos);
  std::string to_proto_body =
      output.substr(to_proto_pos, brace_pos - to_proto_pos);
  EXPECT_THAT(to_proto_body, Not(HasSubstr("pad")));
}

TEST(EmitHeaderTest, FromProtoBody) {
  auto module = MakeModule();

  auto* s = module.add_structs();
  s->set_name("Reg");
  {
    auto* f = s->add_fields();
    f->set_name("flag");
    f->mutable_type()->mutable_bool_();
  }
  {
    auto* f = s->add_fields();
    f->set_name("val");
    f->mutable_type()->mutable_bitvec()->set_width(8);
  }

  auto resolved = Resolve(module);
  std::string output = EmitHeader(resolved, "test.pb.h");

  EXPECT_THAT(output,
              HasSubstr("inline RegConcrete RegConcrete::FromProto("
                        "const RegProto& proto) {"));
  EXPECT_THAT(output,
              HasSubstr("result.flag = z3w::Bool(proto.flag());"));
  EXPECT_THAT(output,
              HasSubstr("std::get<0>(z3w::UInt<8>::Checked(proto.val()))"));
}

TEST(EmitHeaderTest, PackBody) {
  auto module = MakeModule();

  auto* s = module.add_structs();
  s->set_name("Reg");
  {
    auto* f = s->add_fields();
    f->set_name("flag");
    f->mutable_type()->mutable_bool_();
  }
  {
    auto* f = s->add_fields();
    f->set_name("val");
    f->mutable_type()->mutable_bitvec()->set_width(7);
  }

  auto resolved = Resolve(module);
  std::string output = EmitHeader(resolved, "test.pb.h");

  // LSB_FIRST: flag is bit 0, val is bits [7:1]
  // concat(high, low) so: concat(val, as_uint1(flag))
  EXPECT_THAT(output,
              HasSubstr("return z3w::concat(val, z3w::as_uint1(flag));"));
}

TEST(EmitHeaderTest, CreateBody) {
  auto module = MakeModule();

  auto* s = module.add_structs();
  s->set_name("Reg");
  {
    auto* f = s->add_fields();
    f->set_name("flag");
    f->mutable_type()->mutable_bool_();
  }
  {
    auto* f = s->add_fields();
    f->set_name("val");
    f->mutable_type()->mutable_bitvec()->set_width(7);
  }

  auto resolved = Resolve(module);
  std::string output = EmitHeader(resolved, "test.pb.h");

  EXPECT_THAT(output,
              HasSubstr("z3w::SymBool(ctx, prefix + \".flag\")"));
  EXPECT_THAT(output,
              HasSubstr("z3w::SymUInt<7>(ctx, prefix + \".val\")"));
}

TEST(EmitHeaderTest, ArrayInPack) {
  auto module = MakeModule();

  auto* s = module.add_structs();
  s->set_name("Reg");
  {
    auto* f = s->add_fields();
    f->set_name("items");
    f->mutable_type()->mutable_bitvec()->set_width(4);
    f->set_count(3);
  }

  auto resolved = Resolve(module);
  std::string output = EmitHeader(resolved, "test.pb.h");

  // Array elements should be expanded in concat
  EXPECT_THAT(
      output,
      HasSubstr("return z3w::concat(items[2], items[1], items[0]);"));
}

TEST(EmitHeaderTest, MsbFirstPack) {
  auto module = MakeModule();
  module.set_field_pack_order(z3wire_rdl::FIELD_PACK_ORDER_MSB_FIRST);

  auto* s = module.add_structs();
  s->set_name("Reg");
  {
    auto* f = s->add_fields();
    f->set_name("high");
    f->mutable_type()->mutable_bitvec()->set_width(4);
  }
  {
    auto* f = s->add_fields();
    f->set_name("low");
    f->mutable_type()->mutable_bitvec()->set_width(4);
  }

  auto resolved = Resolve(module);
  std::string output = EmitHeader(resolved, "test.pb.h");

  // MSB_FIRST: fields listed high-to-low, concat preserves order
  EXPECT_THAT(output, HasSubstr("return z3w::concat(high, low);"));
  // Bit offsets: high=[7:4], low=[3:0]
  EXPECT_THAT(output, HasSubstr("// [7:4]"));
  EXPECT_THAT(output, HasSubstr("// [3:0]"));
  EXPECT_THAT(output, HasSubstr("field pack order: MSB first"));
}

TEST(EmitHeaderTest, FullExample) {
  auto module = MakeModule("example");
  module.set_file_prefix("status_register");

  // Enum
  {
    auto* e = module.add_enums();
    e->set_name("OpMode");
    e->set_desc("Operating mode");
    e->set_width(2);
    {
      auto* v = e->add_values();
      v->set_name("kIdle");
      v->set_value(0);
    }
    {
      auto* v = e->add_values();
      v->set_name("kActive");
      v->set_value(1);
    }
    {
      auto* v = e->add_values();
      v->set_name("kSleep");
      v->set_value(2);
    }
  }

  // ErrorInfo struct
  {
    auto* s = module.add_structs();
    s->set_name("ErrorInfo");
    s->set_desc("Error information");
    s->set_width(8);
    {
      auto* f = s->add_fields();
      f->set_name("code");
      f->mutable_type()->mutable_bitvec()->set_width(4);
    }
    {
      auto* f = s->add_fields();
      f->set_name("severity");
      f->mutable_type()->mutable_bitvec()->set_width(2);
      f->mutable_type()->mutable_bitvec()->set_signedness(
          z3wire_rdl::SIGNEDNESS_SIGNED);
    }
    {
      auto* f = s->add_fields();
      f->set_name("fatal");
      f->mutable_type()->mutable_bool_();
    }
    {
      auto* f = s->add_fields();
      f->set_name("reserved");
      f->mutable_type()->mutable_bitvec()->set_width(1);
      f->set_reserved(true);
    }
  }

  // StatusRegister struct
  {
    auto* s = module.add_structs();
    s->set_name("StatusRegister");
    s->set_desc("Device status register");
    s->set_width(32);
    {
      auto* f = s->add_fields();
      f->set_name("ready");
      f->mutable_type()->mutable_bool_();
    }
    {
      auto* f = s->add_fields();
      f->set_name("mode");
      f->mutable_type()->set_enum_ref("OpMode");
    }
    {
      auto* f = s->add_fields();
      f->set_name("error");
      f->mutable_type()->set_struct_ref("ErrorInfo");
    }
    {
      auto* f = s->add_fields();
      f->set_name("counters");
      f->mutable_type()->mutable_bitvec()->set_width(4);
      f->set_count(4);
    }
    {
      auto* f = s->add_fields();
      f->set_name("reserved");
      f->mutable_type()->mutable_bitvec()->set_width(5);
      f->set_reserved(true);
    }
  }

  auto resolved = Resolve(module);
  std::string output = EmitHeader(resolved, "status_register.pb.h");

  // Enum
  EXPECT_THAT(output, HasSubstr("struct OpMode {"));
  EXPECT_THAT(output,
              HasSubstr("static constexpr auto kIdle = "
                        "z3w::UInt<2>::Literal<0>();"));
  EXPECT_THAT(output,
              HasSubstr("static constexpr auto kSleep = "
                        "z3w::UInt<2>::Literal<2>();"));

  // ErrorInfo concrete
  EXPECT_THAT(output, HasSubstr("struct ErrorInfoConcrete {"));
  EXPECT_THAT(output, HasSubstr("z3w::UInt<4> code;"));
  EXPECT_THAT(output, HasSubstr("z3w::SInt<2> severity;"));

  // ErrorInfo symbolic
  EXPECT_THAT(output, HasSubstr("struct ErrorInfoSymbolic {"));
  EXPECT_THAT(output, HasSubstr("z3w::SymSInt<2> severity;"));

  // StatusRegister concrete - nested struct and array
  EXPECT_THAT(output, HasSubstr("ErrorInfoConcrete error;"));
  EXPECT_THAT(output, HasSubstr("std::array<z3w::UInt<4>, 4> counters;"));

  // StatusRegister symbolic - bit offsets
  EXPECT_THAT(output, HasSubstr("z3w::SymUInt<32> Pack() const;"));
  EXPECT_THAT(output, HasSubstr("ErrorInfoSymbolic error;"));
  EXPECT_THAT(output,
              HasSubstr("std::array<z3w::SymUInt<4>, 4> counters;"));

  // Namespace
  EXPECT_THAT(output, HasSubstr("namespace example {"));
  EXPECT_THAT(output, HasSubstr("}  // namespace example"));

  // Section structure
  EXPECT_THAT(output, HasSubstr("// Enum constants"));
  EXPECT_THAT(output, HasSubstr("// Concrete types"));
  EXPECT_THAT(output, HasSubstr("// Symbolic types"));
  EXPECT_THAT(output, HasSubstr("// Inline implementations"));

  // Sections in correct order
  auto enum_pos = output.find("// Enum constants");
  auto concrete_pos = output.find("// Concrete types");
  auto symbolic_pos = output.find("// Symbolic types");
  auto impl_pos = output.find("// Inline implementations");
  EXPECT_LT(enum_pos, concrete_pos);
  EXPECT_LT(concrete_pos, symbolic_pos);
  EXPECT_LT(symbolic_pos, impl_pos);

  // Types are in the right sections
  EXPECT_LT(concrete_pos, output.find("struct ErrorInfoConcrete {"));
  EXPECT_LT(output.find("struct ErrorInfoConcrete {"), symbolic_pos);
  EXPECT_LT(symbolic_pos, output.find("struct ErrorInfoSymbolic {"));
  EXPECT_LT(output.find("struct ErrorInfoSymbolic {"), impl_pos);
}

}  // namespace
}  // namespace z3wire::weave
