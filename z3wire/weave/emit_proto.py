"""Generates a .proto file from a resolved RDL module."""

from z3wire.weave.resolver import ResolvedModule


def _proto_type(field) -> str:
    """Map a resolved field to its proto type."""
    if field.kind == "bool":
        return "bool"
    elif field.kind == "bitvec":
        if field.signedness == "signed":
            return "sint64" if field.element_width > 32 else "sint32"
        return "uint64" if field.element_width > 32 else "uint32"
    elif field.kind == "enum_ref":
        # Enum fields use the underlying integer type
        return "uint64" if field.element_width > 32 else "uint32"
    elif field.kind == "struct_ref":
        return f"{field.struct_name}Proto"
    else:
        raise ValueError(f"Unknown field kind: {field.kind}")


def emit_proto(module: ResolvedModule) -> str:
    """Generate a .proto file string from a resolved module."""
    lines = []
    lines.append('syntax = "proto3";')
    lines.append(f"package {module.namespace};")
    lines.append("")

    for struct in module.structs:
        if struct.desc:
            lines.append(f"// {struct.desc}")
        lines.append(f"message {struct.name}Proto {{")
        field_number = 1
        for field in struct.fields:
            if field.reserved:
                continue
            proto_type = _proto_type(field)
            if field.count >= 1:
                lines.append(f"  repeated {proto_type} {field.name} = {field_number};")
            else:
                lines.append(f"  {proto_type} {field.name} = {field_number};")
            field_number += 1
        lines.append("}")
        lines.append("")

    return "\n".join(lines)
