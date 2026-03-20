"""Resolves an RDL Module: validates, computes widths and bit offsets."""

from dataclasses import dataclass

from z3wire.weave import rdl_pb2

_RESERVED_NAMES = {
    "Create",
    "Pack",
    "ToConcrete",
    "FromConcrete",
    "ToProto",
    "FromProto",
}


@dataclass
class ResolvedField:
    name: str
    desc: str
    width: int  # Total width in bits (including count)
    offset: int  # Bit offset within parent struct
    count: int  # 0 = scalar, >= 1 = array
    element_width: int  # Width of a single element
    reserved: bool
    # Type info
    kind: str  # "bool", "bitvec", "enum_ref", "struct_ref"
    signedness: str  # "unsigned" or "signed" (for bitvec)
    enum_name: str = ""
    struct_name: str = ""


@dataclass
class ResolvedEnum:
    name: str
    desc: str
    width: int
    values: list  # list of (name, desc, value) tuples


@dataclass
class ResolvedStruct:
    name: str
    desc: str
    total_width: int
    field_pack_order: str  # "lsb_first"
    fields: list  # list of ResolvedField


@dataclass
class ResolvedModule:
    file_prefix: str
    namespace: str
    enums: list  # list of ResolvedEnum
    structs: list  # list of ResolvedStruct


def resolve(module: rdl_pb2.Module) -> ResolvedModule:
    """Resolve an RDL module: validate and compute layouts."""
    if not module.file_prefix:
        raise ValueError("Module must specify file_prefix")
    enums = _resolve_enums(module)
    enum_map = {e.name: e for e in enums}
    structs = _resolve_structs(module, enum_map)
    return ResolvedModule(
        file_prefix=module.file_prefix,
        namespace=module.namespace,
        enums=enums,
        structs=structs,
    )


def _resolve_enums(module):
    resolved = []
    for enum_def in module.enums:
        # Validate values fit within width
        max_val = (1 << enum_def.width) - 1
        for v in enum_def.values:
            if v.value > max_val:
                raise ValueError(
                    f"Enum '{enum_def.name}': value '{v.name}' = {v.value} "
                    f"exceeds width {enum_def.width} (max {max_val})"
                )
        # Validate unique values
        seen_values = set()
        for v in enum_def.values:
            if v.value in seen_values:
                raise ValueError(
                    f"Enum '{enum_def.name}': duplicate value {v.value}"
                )
            seen_values.add(v.value)

        values = [(v.name, v.desc, v.value) for v in enum_def.values]
        resolved.append(
            ResolvedEnum(
                name=enum_def.name,
                desc=enum_def.desc,
                width=enum_def.width,
                values=values,
            )
        )
    return resolved


def _field_element_width(field_type, enum_map, struct_map):
    kind = field_type.WhichOneof("kind")
    if kind == "bool":
        return 1
    elif kind == "bitvec":
        return field_type.bitvec.width
    elif kind == "enum_ref":
        if field_type.enum_ref not in enum_map:
            raise ValueError(f"Unknown enum_ref '{field_type.enum_ref}'")
        return enum_map[field_type.enum_ref].width
    elif kind == "struct_ref":
        return struct_map[field_type.struct_ref].total_width
    else:
        raise ValueError(f"Unknown field type: {kind}")


def _resolve_pack_order(module, struct):
    order = struct.field_pack_order
    if order == rdl_pb2.FIELD_PACK_ORDER_UNSPECIFIED:
        order = module.field_pack_order
    if order == rdl_pb2.FIELD_PACK_ORDER_UNSPECIFIED:
        raise ValueError(
            f"Struct '{struct.name}': field_pack_order not specified "
            f"at struct or module level"
        )
    if order == rdl_pb2.FIELD_PACK_ORDER_LSB_FIRST:
        return "lsb_first"
    return "msb_first"


def _resolve_structs(module, enum_map):
    struct_map = {}
    resolved = []

    # Sort structs so dependencies come first.
    pending = list(module.structs)
    max_iterations = len(pending) * len(pending) + 1
    iteration = 0
    while pending:
        iteration += 1
        if iteration > max_iterations:
            names = [s.name for s in pending]
            raise ValueError(f"Circular struct dependency: {names}")
        struct = pending.pop(0)
        # Check if all struct_ref dependencies are resolved
        deps_met = True
        for f in struct.fields:
            kind = f.type.WhichOneof("kind")
            if kind == "struct_ref" and f.type.struct_ref not in struct_map:
                deps_met = False
                break
        if not deps_met:
            pending.append(struct)
            continue

        pack_order = _resolve_pack_order(module, struct)

        # Validate field names
        for f in struct.fields:
            if f.name in _RESERVED_NAMES:
                raise ValueError(
                    f"Struct '{struct.name}': field name '{f.name}' is "
                    f"reserved (collides with generated method)"
                )
            kind = f.type.WhichOneof("kind")
            if kind == "enum_ref" and f.type.enum_ref not in enum_map:
                raise ValueError(
                    f"Struct '{struct.name}', field '{f.name}': "
                    f"unknown enum_ref '{f.type.enum_ref}'"
                )

        # First pass: compute field widths.
        field_infos = []
        for f in struct.fields:
            kind = f.type.WhichOneof("kind")
            element_width = _field_element_width(f.type, enum_map, struct_map)
            count = f.count
            total_width = element_width * max(count, 1)

            signedness = "unsigned"
            if (
                kind == "bitvec"
                and f.type.bitvec.signedness == rdl_pb2.SIGNEDNESS_SIGNED
            ):
                signedness = "signed"

            field_infos.append(
                (f, kind, element_width, count, total_width, signedness)
            )

        struct_total_width = sum(fw for _, _, _, _, fw, _ in field_infos)

        # Second pass: assign offsets based on pack order.
        fields = []
        if pack_order == "lsb_first":
            offset = 0
            for (
                f,
                kind,
                element_width,
                count,
                total_width,
                signedness,
            ) in field_infos:
                fields.append(
                    ResolvedField(
                        name=f.name,
                        desc=f.desc,
                        width=total_width,
                        offset=offset,
                        count=count,
                        element_width=element_width,
                        reserved=f.reserved,
                        kind=kind,
                        signedness=signedness,
                        enum_name=(
                            f.type.enum_ref if kind == "enum_ref" else ""
                        ),
                        struct_name=(
                            f.type.struct_ref if kind == "struct_ref" else ""
                        ),
                    )
                )
                offset += total_width
        else:
            # MSB_FIRST: first field in list occupies highest bits.
            offset = struct_total_width
            for (
                f,
                kind,
                element_width,
                count,
                total_width,
                signedness,
            ) in field_infos:
                offset -= total_width
                fields.append(
                    ResolvedField(
                        name=f.name,
                        desc=f.desc,
                        width=total_width,
                        offset=offset,
                        count=count,
                        element_width=element_width,
                        reserved=f.reserved,
                        kind=kind,
                        signedness=signedness,
                        enum_name=(
                            f.type.enum_ref if kind == "enum_ref" else ""
                        ),
                        struct_name=(
                            f.type.struct_ref if kind == "struct_ref" else ""
                        ),
                    )
                )

        total_width = struct_total_width
        if struct.HasField("width") and struct.width != total_width:
            raise ValueError(
                f"Struct '{struct.name}': declared width {struct.width} "
                f"but fields sum to {total_width}"
            )

        rs = ResolvedStruct(
            name=struct.name,
            desc=struct.desc,
            total_width=total_width,
            field_pack_order=pack_order,
            fields=fields,
        )
        struct_map[struct.name] = rs
        resolved.append(rs)

    return resolved
