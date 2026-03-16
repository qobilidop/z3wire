"""Tests for the header emitter."""

import unittest

from z3wire.weave import rdl_pb2
from z3wire.weave.resolver import resolve
from z3wire.weave.emit_header import emit_header


class EmitHeaderTest(unittest.TestCase):
    def test_enum_constants(self):
        module = rdl_pb2.Module(
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            enums=[
                rdl_pb2.EnumDef(
                    name="OpMode",
                    desc="Operating mode",
                    width=2,
                    values=[
                        rdl_pb2.EnumValue(name="kIdle", value=0),
                        rdl_pb2.EnumValue(name="kActive", value=1),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_header(resolved, proto_header="test.pb.h")

        self.assertIn("#pragma once", output)
        self.assertIn("namespace test {", output)
        self.assertIn("struct OpMode {", output)
        self.assertIn("static constexpr z3w::UInt<2> kIdle{0};", output)
        self.assertIn("static constexpr z3w::UInt<2> kActive{1};", output)
        self.assertIn("}  // namespace test", output)

    def test_concrete_struct(self):
        module = rdl_pb2.Module(
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="Reg",
                    desc="A register",
                    fields=[
                        rdl_pb2.Field(
                            name="flag",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                        rdl_pb2.Field(
                            name="val",
                            type=rdl_pb2.FieldType(
                                bitvec=rdl_pb2.BitVecType(
                                    width=4,
                                    signedness=rdl_pb2.SIGNEDNESS_SIGNED,
                                )
                            ),
                        ),
                        rdl_pb2.Field(
                            name="items",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=8)),
                            count=2,
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_header(resolved, proto_header="test.pb.h")

        self.assertIn("struct RegConcrete {", output)
        self.assertIn("bool flag;", output)
        self.assertIn("z3w::SInt<4> val;", output)
        self.assertIn("std::array<z3w::UInt<8>, 2> items;", output)
        self.assertIn("RegProto ToProto() const;", output)
        self.assertIn("static RegConcrete FromProto(const RegProto& proto);", output)

    def test_symbolic_struct(self):
        module = rdl_pb2.Module(
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="Reg",
                    fields=[
                        rdl_pb2.Field(
                            name="flag",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                        rdl_pb2.Field(
                            name="val",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=7)),
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_header(resolved, proto_header="test.pb.h")

        self.assertIn("struct RegSymbolic {", output)
        self.assertIn("z3w::Bool flag;", output)
        self.assertIn("z3w::Ubv<7> val;", output)
        self.assertIn("// [0]", output)
        self.assertIn("// [7:1]", output)
        self.assertIn("z3w::Ubv<8> Pack() const;", output)
        self.assertIn("static RegSymbolic Create(z3::context& ctx,", output)

    def test_to_proto_body(self):
        module = rdl_pb2.Module(
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="Reg",
                    fields=[
                        rdl_pb2.Field(
                            name="flag",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                        rdl_pb2.Field(
                            name="val",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=8)),
                        ),
                        rdl_pb2.Field(
                            name="pad",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=7)),
                            reserved=True,
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_header(resolved, proto_header="test.pb.h")

        self.assertIn("inline RegProto RegConcrete::ToProto() const {", output)
        self.assertIn("proto.set_flag(flag);", output)
        self.assertIn("proto.set_val(static_cast<uint32_t>(val.value()));", output)
        # Reserved field should not appear in ToProto
        self.assertNotIn("pad", output.split("ToProto")[1].split("}")[0])

    def test_from_proto_body(self):
        module = rdl_pb2.Module(
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="Reg",
                    fields=[
                        rdl_pb2.Field(
                            name="flag",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                        rdl_pb2.Field(
                            name="val",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=8)),
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_header(resolved, proto_header="test.pb.h")

        self.assertIn(
            "inline RegConcrete RegConcrete::FromProto(const RegProto& proto) {", output
        )
        self.assertIn("result.flag = proto.flag();", output)
        self.assertIn("std::get<0>(z3w::UInt<8>::checked(proto.val()))", output)

    def test_pack_body(self):
        module = rdl_pb2.Module(
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="Reg",
                    fields=[
                        rdl_pb2.Field(
                            name="flag",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                        rdl_pb2.Field(
                            name="val",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=7)),
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_header(resolved, proto_header="test.pb.h")

        # LSB_FIRST: flag is bit 0, val is bits [7:1]
        # concat(high, low) so: concat(val, to_ubv1(flag))
        self.assertIn("return z3w::concat(val, z3w::to_ubv1(flag));", output)

    def test_create_body(self):
        module = rdl_pb2.Module(
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="Reg",
                    fields=[
                        rdl_pb2.Field(
                            name="flag",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                        rdl_pb2.Field(
                            name="val",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=7)),
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_header(resolved, proto_header="test.pb.h")

        self.assertIn('z3w::Bool(ctx, prefix + ".flag")', output)
        self.assertIn('z3w::Ubv<7>(ctx, prefix + ".val")', output)

    def test_array_in_pack(self):
        module = rdl_pb2.Module(
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="Reg",
                    fields=[
                        rdl_pb2.Field(
                            name="items",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=4)),
                            count=3,
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_header(resolved, proto_header="test.pb.h")

        # Array elements should be expanded in concat
        self.assertIn("return z3w::concat(items[2], items[1], items[0]);", output)

    def test_full_example(self):
        """Test the complete StatusRegister example from the design doc."""
        module = rdl_pb2.Module(
            namespace="example",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            enums=[
                rdl_pb2.EnumDef(
                    name="OpMode",
                    desc="Operating mode",
                    width=2,
                    values=[
                        rdl_pb2.EnumValue(name="kIdle", value=0),
                        rdl_pb2.EnumValue(name="kActive", value=1),
                        rdl_pb2.EnumValue(name="kSleep", value=2),
                    ],
                ),
            ],
            structs=[
                rdl_pb2.Struct(
                    name="ErrorInfo",
                    desc="Error information",
                    width=8,
                    fields=[
                        rdl_pb2.Field(
                            name="code",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=4)),
                        ),
                        rdl_pb2.Field(
                            name="severity",
                            type=rdl_pb2.FieldType(
                                bitvec=rdl_pb2.BitVecType(
                                    width=2,
                                    signedness=rdl_pb2.SIGNEDNESS_SIGNED,
                                )
                            ),
                        ),
                        rdl_pb2.Field(
                            name="fatal",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                        rdl_pb2.Field(
                            name="reserved",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=1)),
                            reserved=True,
                        ),
                    ],
                ),
                rdl_pb2.Struct(
                    name="StatusRegister",
                    desc="Device status register",
                    width=32,
                    fields=[
                        rdl_pb2.Field(
                            name="ready",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                        rdl_pb2.Field(
                            name="mode",
                            type=rdl_pb2.FieldType(enum_ref="OpMode"),
                        ),
                        rdl_pb2.Field(
                            name="error",
                            type=rdl_pb2.FieldType(struct_ref="ErrorInfo"),
                        ),
                        rdl_pb2.Field(
                            name="counters",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=4)),
                            count=4,
                        ),
                        rdl_pb2.Field(
                            name="reserved",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=5)),
                            reserved=True,
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_header(resolved, proto_header="status_register.pb.h")

        # Enum
        self.assertIn("struct OpMode {", output)
        self.assertIn("static constexpr z3w::UInt<2> kIdle{0};", output)
        self.assertIn("static constexpr z3w::UInt<2> kSleep{2};", output)

        # ErrorInfo concrete
        self.assertIn("struct ErrorInfoConcrete {", output)
        self.assertIn("z3w::UInt<4> code;", output)
        self.assertIn("z3w::SInt<2> severity;", output)

        # ErrorInfo symbolic
        self.assertIn("struct ErrorInfoSymbolic {", output)
        self.assertIn("z3w::Sbv<2> severity;", output)

        # StatusRegister concrete — nested struct and array
        self.assertIn("ErrorInfoConcrete error;", output)
        self.assertIn("std::array<z3w::UInt<4>, 4> counters;", output)

        # StatusRegister symbolic — bit offsets
        self.assertIn("z3w::Ubv<32> Pack() const;", output)
        self.assertIn("ErrorInfoSymbolic error;", output)
        self.assertIn("std::array<z3w::Ubv<4>, 4> counters;", output)

        # Namespace
        self.assertIn("namespace example {", output)
        self.assertIn("}  // namespace example", output)

        # Section structure
        self.assertIn("// Enum constants", output)
        self.assertIn("// Concrete types", output)
        self.assertIn("// Symbolic types", output)
        self.assertIn("// Inline implementations", output)

        # Sections in correct order
        enum_pos = output.index("// Enum constants")
        concrete_pos = output.index("// Concrete types")
        symbolic_pos = output.index("// Symbolic types")
        impl_pos = output.index("// Inline implementations")
        self.assertLess(enum_pos, concrete_pos)
        self.assertLess(concrete_pos, symbolic_pos)
        self.assertLess(symbolic_pos, impl_pos)

        # Types are in the right sections
        self.assertLess(concrete_pos, output.index("struct ErrorInfoConcrete {"))
        self.assertLess(output.index("struct ErrorInfoConcrete {"), symbolic_pos)
        self.assertLess(symbolic_pos, output.index("struct ErrorInfoSymbolic {"))
        self.assertLess(output.index("struct ErrorInfoSymbolic {"), impl_pos)


if __name__ == "__main__":
    unittest.main()
