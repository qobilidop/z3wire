"""Tests for the proto emitter."""

import unittest

from z3wire.weave import rdl_pb2
from z3wire.weave.emit_proto import emit_proto
from z3wire.weave.resolver import resolve


class EmitProtoTest(unittest.TestCase):
    def test_simple_struct(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="Simple",
                    desc="A simple struct",
                    fields=[
                        rdl_pb2.Field(
                            name="flag",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                        rdl_pb2.Field(
                            name="value",
                            type=rdl_pb2.FieldType(
                                bitvec=rdl_pb2.BitVecType(width=8)
                            ),
                        ),
                        rdl_pb2.Field(
                            name="pad",
                            type=rdl_pb2.FieldType(
                                bitvec=rdl_pb2.BitVecType(width=7)
                            ),
                            reserved=True,
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_proto(resolved)

        self.assertIn('syntax = "proto3";', output)
        self.assertIn("package test;", output)
        self.assertIn("message SimpleProto", output)
        self.assertIn("bool flag = 1;", output)
        self.assertIn("uint32 value = 2;", output)
        # Reserved field should be omitted
        self.assertNotIn("pad", output)

    def test_nested_and_signed(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="example",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="Inner",
                    fields=[
                        rdl_pb2.Field(
                            name="val",
                            type=rdl_pb2.FieldType(
                                bitvec=rdl_pb2.BitVecType(
                                    width=4,
                                    signedness=rdl_pb2.SIGNEDNESS_SIGNED,
                                )
                            ),
                        ),
                    ],
                ),
                rdl_pb2.Struct(
                    name="Outer",
                    fields=[
                        rdl_pb2.Field(
                            name="inner",
                            type=rdl_pb2.FieldType(struct_ref="Inner"),
                        ),
                        rdl_pb2.Field(
                            name="data",
                            type=rdl_pb2.FieldType(
                                bitvec=rdl_pb2.BitVecType(width=4)
                            ),
                            count=3,
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_proto(resolved)

        self.assertIn("InnerProto", output)
        self.assertIn("sint32 val = 1;", output)
        self.assertIn("InnerProto inner = 1;", output)
        self.assertIn("repeated uint32 data = 2;", output)

    def test_enum_ref_field(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            enums=[
                rdl_pb2.EnumDef(
                    name="Mode",
                    width=2,
                    values=[
                        rdl_pb2.EnumValue(name="kA", value=0),
                        rdl_pb2.EnumValue(name="kB", value=1),
                    ],
                ),
            ],
            structs=[
                rdl_pb2.Struct(
                    name="S",
                    fields=[
                        rdl_pb2.Field(
                            name="mode",
                            type=rdl_pb2.FieldType(enum_ref="Mode"),
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        output = emit_proto(resolved)

        self.assertIn("uint32 mode = 1;", output)


if __name__ == "__main__":
    unittest.main()
