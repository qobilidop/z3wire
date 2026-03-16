"""Generates a C++ header from a resolved RDL module."""

from z3wire.weave.resolver import ResolvedModule, ResolvedField


def _symbolic_type(field: ResolvedField) -> str:
    """Return the Z3Wire symbolic C++ type for a field."""
    if field.kind == "bool":
        return "z3w::Bool"
    elif field.kind == "bitvec" or field.kind == "enum_ref":
        w = field.element_width
        if field.signedness == "signed":
            return f"z3w::Sbv<{w}>"
        return f"z3w::Ubv<{w}>"
    elif field.kind == "struct_ref":
        return f"{field.struct_name}Symbolic"
    raise ValueError(f"Unknown kind: {field.kind}")


def _concrete_type(field: ResolvedField) -> str:
    """Return the Z3Wire concrete C++ type for a field."""
    if field.kind == "bool":
        return "bool"
    elif field.kind == "bitvec" or field.kind == "enum_ref":
        w = field.element_width
        if field.signedness == "signed":
            return f"z3w::SInt<{w}>"
        return f"z3w::UInt<{w}>"
    elif field.kind == "struct_ref":
        return f"{field.struct_name}Concrete"
    raise ValueError(f"Unknown kind: {field.kind}")


def _field_decl(type_str: str, field: ResolvedField) -> str:
    """Generate a field declaration, with std::array for arrays."""
    if field.count >= 1:
        return f"std::array<{type_str}, {field.count}> {field.name};"
    return f"{type_str} {field.name};"


def emit_header(module: ResolvedModule, proto_header: str) -> str:
    """Generate a C++ header string from a resolved module."""
    lines = []

    # Includes
    lines.append("#pragma once")
    lines.append("")
    has_arrays = any(f.count >= 1 for s in module.structs for f in s.fields)
    if has_arrays:
        lines.append("#include <array>")
    lines.append("#include <string>")
    lines.append("")
    lines.append('#include "z3wire/bool.h"')
    lines.append('#include "z3wire/bitvec.h"')
    lines.append('#include "z3wire/int.h"')
    lines.append(f'#include "{proto_header}"')
    lines.append("")
    lines.append(f"namespace {module.namespace} {{")
    lines.append("")

    # Enum constants
    for enum in module.enums:
        if enum.desc:
            lines.append(f"// {enum.desc} (width: {enum.width})")
        lines.append(f"struct {enum.name} {{")
        for name, desc, value in enum.values:
            lines.append(
                f"  static constexpr z3w::UInt<{enum.width}> {name}{{{value}}};"
            )
        lines.append("};")
        lines.append("")

    # Concrete and symbolic structs
    for struct in module.structs:
        # Concrete struct
        if struct.desc:
            lines.append(f"// {struct.desc}")
        lines.append(f"struct {struct.name}Concrete {{")
        for field in struct.fields:
            lines.append(f"  {_field_decl(_concrete_type(field), field)}")
        lines.append("")
        lines.append(f"  {struct.name}Proto ToProto() const;")
        lines.append(
            f"  static {struct.name}Concrete FromProto("
            f"const {struct.name}Proto& proto);"
        )
        lines.append("};")
        lines.append("")

        # Symbolic struct
        if struct.desc:
            lines.append(f"// {struct.desc} (symbolic)")
        pack_label = "LSB" if struct.field_pack_order == "lsb_first" else "MSB"
        lines.append(
            f"// Total width: {struct.total_width} bits, "
            f"field pack order: {pack_label} first"
        )
        lines.append(f"struct {struct.name}Symbolic {{")
        for field in struct.fields:
            decl = _field_decl(_symbolic_type(field), field)
            high = field.offset + field.width - 1
            if field.width == 1:
                bit_comment = f"  // [{field.offset}]"
            else:
                bit_comment = f"  // [{high}:{field.offset}]"
            lines.append(f"  {decl}{bit_comment}")
        lines.append("")
        lines.append(f"  static {struct.name}Symbolic Create(z3::context& ctx,")
        lines.append(f"      const std::string& prefix);")
        lines.append(f"  z3w::Ubv<{struct.total_width}> Pack() const;")
        lines.append(
            f"  {struct.name}Concrete ToConcrete(" f"const z3::model& model) const;"
        )
        lines.append(f"  static {struct.name}Symbolic FromConcrete(z3::context& ctx,")
        lines.append(f"      const {struct.name}Concrete& concrete);")
        lines.append("};")
        lines.append("")

    lines.append(f"}}  // namespace {module.namespace}")
    lines.append("")

    return "\n".join(lines)
