"""Tests for the RDL resolver."""

import re
import unittest

from z3wire.weave import rdl_pb2
from z3wire.weave.resolver import resolve


class ResolverTest(unittest.TestCase):
    def test_bool_field_width(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="Simple",
                    fields=[
                        rdl_pb2.Field(
                            name="flag",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        self.assertEqual(resolved.structs[0].total_width, 1)
        self.assertEqual(resolved.structs[0].fields[0].width, 1)
        self.assertEqual(resolved.structs[0].fields[0].offset, 0)

    def test_bitvec_field_width(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="S",
                    fields=[
                        rdl_pb2.Field(
                            name="a",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=8)),
                        ),
                        rdl_pb2.Field(
                            name="b",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=4)),
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        self.assertEqual(resolved.structs[0].total_width, 12)
        self.assertEqual(resolved.structs[0].fields[0].offset, 0)
        self.assertEqual(resolved.structs[0].fields[1].offset, 8)

    def test_count_zero_is_scalar_count_one_is_array(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="S",
                    fields=[
                        rdl_pb2.Field(
                            name="scalar",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=8)),
                            count=0,
                        ),
                        rdl_pb2.Field(
                            name="array",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=8)),
                            count=1,
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        self.assertEqual(resolved.structs[0].fields[0].count, 0)
        self.assertEqual(resolved.structs[0].fields[0].width, 8)
        self.assertEqual(resolved.structs[0].fields[1].count, 1)
        self.assertEqual(resolved.structs[0].fields[1].width, 8)

    def test_unresolved_enum_ref(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="S",
                    fields=[
                        rdl_pb2.Field(
                            name="x",
                            type=rdl_pb2.FieldType(enum_ref="NoSuchEnum"),
                        ),
                    ],
                ),
            ],
        )
        with self.assertRaisesRegex(ValueError, "NoSuchEnum"):
            resolve(module)

    def test_unresolved_struct_ref(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="S",
                    fields=[
                        rdl_pb2.Field(
                            name="x",
                            type=rdl_pb2.FieldType(struct_ref="NoSuchStruct"),
                        ),
                    ],
                ),
            ],
        )
        with self.assertRaisesRegex(ValueError, "Circular struct dependency"):
            resolve(module)

    def test_enum_value_exceeds_width(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            enums=[
                rdl_pb2.EnumDef(
                    name="E",
                    width=2,
                    values=[rdl_pb2.EnumValue(name="kBig", value=4)],
                ),
            ],
        )
        with self.assertRaisesRegex(ValueError, "exceeds width"):
            resolve(module)

    def test_duplicate_enum_values(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            enums=[
                rdl_pb2.EnumDef(
                    name="E",
                    width=2,
                    values=[
                        rdl_pb2.EnumValue(name="kA", value=0),
                        rdl_pb2.EnumValue(name="kB", value=0),
                    ],
                ),
            ],
        )
        with self.assertRaisesRegex(ValueError, re.compile("[Dd]uplicate")):
            resolve(module)

    def test_width_mismatch(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="S",
                    width=16,
                    fields=[
                        rdl_pb2.Field(
                            name="x",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                    ],
                ),
            ],
        )
        with self.assertRaisesRegex(ValueError, r"declared width 16.*sum to 1"):
            resolve(module)

    def test_missing_field_pack_order(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            structs=[
                rdl_pb2.Struct(
                    name="S",
                    fields=[
                        rdl_pb2.Field(
                            name="x",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                    ],
                ),
            ],
        )
        with self.assertRaisesRegex(ValueError, "field_pack_order"):
            resolve(module)

    def test_missing_file_prefix(self):
        module = rdl_pb2.Module(
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
        )
        with self.assertRaisesRegex(ValueError, "file_prefix"):
            resolve(module)

    def test_circular_struct_dependency(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="A",
                    fields=[
                        rdl_pb2.Field(
                            name="b",
                            type=rdl_pb2.FieldType(struct_ref="B"),
                        )
                    ],
                ),
                rdl_pb2.Struct(
                    name="B",
                    fields=[
                        rdl_pb2.Field(
                            name="a",
                            type=rdl_pb2.FieldType(struct_ref="A"),
                        )
                    ],
                ),
            ],
        )
        with self.assertRaisesRegex(ValueError, re.compile("[Cc]ircular")):
            resolve(module)

    def test_msb_first_offsets(self):
        """MSB_FIRST: first field in list gets highest bit offset."""
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_MSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="S",
                    width=8,
                    fields=[
                        rdl_pb2.Field(
                            name="high",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=3)),
                        ),
                        rdl_pb2.Field(
                            name="mid",
                            type=rdl_pb2.FieldType(bitvec=rdl_pb2.BitVecType(width=4)),
                        ),
                        rdl_pb2.Field(
                            name="low",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                    ],
                ),
            ],
        )
        resolved = resolve(module)
        s = resolved.structs[0]
        self.assertEqual(s.total_width, 8)
        self.assertEqual(s.field_pack_order, "msb_first")
        # high occupies [7:5], mid occupies [4:1], low occupies [0]
        self.assertEqual(s.fields[0].name, "high")
        self.assertEqual(s.fields[0].offset, 5)
        self.assertEqual(s.fields[0].width, 3)
        self.assertEqual(s.fields[1].name, "mid")
        self.assertEqual(s.fields[1].offset, 1)
        self.assertEqual(s.fields[1].width, 4)
        self.assertEqual(s.fields[2].name, "low")
        self.assertEqual(s.fields[2].offset, 0)
        self.assertEqual(s.fields[2].width, 1)

    def test_reserved_field_name_collision(self):
        module = rdl_pb2.Module(
            file_prefix="test",
            namespace="test",
            field_pack_order=rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST,
            structs=[
                rdl_pb2.Struct(
                    name="S",
                    fields=[
                        rdl_pb2.Field(
                            name="Pack",
                            type=rdl_pb2.FieldType(bool=rdl_pb2.BoolType()),
                        ),
                    ],
                ),
            ],
        )
        with self.assertRaisesRegex(ValueError, "Pack.*reserved"):
            resolve(module)

    def test_full_example(self):
        """Test the complete StatusRegister example from the design doc."""
        module = rdl_pb2.Module(
            file_prefix="status_register",
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

        self.assertEqual(resolved.namespace, "example")

        # Enum
        self.assertEqual(len(resolved.enums), 1)
        self.assertEqual(resolved.enums[0].name, "OpMode")
        self.assertEqual(resolved.enums[0].width, 2)

        # ErrorInfo: 4 + 2 + 1 + 1 = 8
        ei = resolved.structs[0]
        self.assertEqual(ei.name, "ErrorInfo")
        self.assertEqual(ei.total_width, 8)
        self.assertEqual(ei.fields[0].offset, 0)  # code [3:0]
        self.assertEqual(ei.fields[1].offset, 4)  # severity [5:4]
        self.assertEqual(ei.fields[2].offset, 6)  # fatal [6]
        self.assertEqual(ei.fields[3].offset, 7)  # reserved [7]

        # StatusRegister: 1 + 2 + 8 + 16 + 5 = 32
        sr = resolved.structs[1]
        self.assertEqual(sr.name, "StatusRegister")
        self.assertEqual(sr.total_width, 32)
        self.assertEqual(sr.fields[0].offset, 0)  # ready [0]
        self.assertEqual(sr.fields[1].offset, 1)  # mode [2:1]
        self.assertEqual(sr.fields[2].offset, 3)  # error [10:3]
        self.assertEqual(sr.fields[3].offset, 11)  # counters [26:11]
        self.assertEqual(sr.fields[4].offset, 27)  # reserved [31:27]

        # Counters: array of 4 x 4-bit
        self.assertEqual(sr.fields[3].count, 4)
        self.assertEqual(sr.fields[3].element_width, 4)
        self.assertEqual(sr.fields[3].width, 16)


if __name__ == "__main__":
    unittest.main()
