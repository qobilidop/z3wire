"""Generates a C++ header from a resolved RDL module."""

from z3wire.weave.resolver import ResolvedField, ResolvedModule, ResolvedStruct


def _symbolic_type(field: ResolvedField) -> str:
    """Return the Z3Wire symbolic C++ type for a field."""
    if field.kind == "bool":
        return "z3w::SymBool"
    elif field.kind == "bitvec" or field.kind == "enum_ref":
        w = field.element_width
        if field.signedness == "signed":
            return f"z3w::SymSInt<{w}>"
        return f"z3w::SymUInt<{w}>"
    elif field.kind == "struct_ref":
        return f"{field.struct_name}Symbolic"
    raise ValueError(f"Unknown kind: {field.kind}")


def _concrete_type(field: ResolvedField) -> str:
    """Return the Z3Wire concrete C++ type for a field."""
    if field.kind == "bool":
        return "z3w::Bool"
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


def _proto_value_type(field: ResolvedField) -> str:
    """Return the C++ type used in the proto message for a field."""
    if field.kind == "bool":
        return "bool"
    elif field.kind == "bitvec" or field.kind == "enum_ref":
        if field.signedness == "signed":
            return "int64_t" if field.element_width > 32 else "int32_t"
        return "uint64_t" if field.element_width > 32 else "uint32_t"
    elif field.kind == "struct_ref":
        return f"{field.struct_name}Proto"
    raise ValueError(f"Unknown kind: {field.kind}")


def _to_proto_expr(field: ResolvedField, accessor: str) -> str:
    """Generate expression to convert a concrete field value to proto value."""
    if field.kind == "bool":
        return f"bool({accessor})"
    elif field.kind == "bitvec" or field.kind == "enum_ref":
        return f"static_cast<{_proto_value_type(field)}>({accessor}.value())"
    elif field.kind == "struct_ref":
        return f"{accessor}.ToProto()"
    raise ValueError(f"Unknown kind: {field.kind}")


def _from_proto_expr(field: ResolvedField, accessor: str) -> str:
    """Generate expression to convert a proto value to concrete field value."""
    if field.kind == "bool":
        return f"z3w::Bool({accessor})"
    elif field.kind == "bitvec" or field.kind == "enum_ref":
        ctype = _concrete_type(field)
        return f"std::get<0>({ctype}::Checked({accessor}))"
    elif field.kind == "struct_ref":
        ctype = _concrete_type(field)
        return f"{ctype}::FromProto({accessor})"
    raise ValueError(f"Unknown kind: {field.kind}")


def _to_concrete_expr(field: ResolvedField, accessor: str) -> str:
    """Generate expression to evaluate a symbolic field to concrete."""
    if field.kind == "bool":
        return f"z3w::Bool(model.eval({accessor}.expr(), true).is_true())"
    elif field.kind == "bitvec" or field.kind == "enum_ref":
        ctype = _concrete_type(field)
        return (
            f"std::get<0>({ctype}::Checked("
            f"static_cast<{_proto_value_type(field)}>("
            f"model.eval({accessor}.expr(), true).get_numeral_int64())))"
        )
    elif field.kind == "struct_ref":
        return f"{accessor}.ToConcrete(model)"
    raise ValueError(f"Unknown kind: {field.kind}")


def _from_concrete_expr(field: ResolvedField, accessor: str) -> str:
    """Generate expression to create symbolic constant from concrete value."""
    if field.kind == "bool":
        return f"z3w::to_symbolic({accessor}, ctx)"
    elif field.kind == "bitvec" or field.kind == "enum_ref":
        return f"z3w::to_symbolic({accessor}, ctx)"
    elif field.kind == "struct_ref":
        stype = _symbolic_type(field)
        return f"{stype}::FromConcrete(ctx, {accessor})"
    raise ValueError(f"Unknown kind: {field.kind}")


def _create_expr(field: ResolvedField, prefix_expr: str) -> str:
    """Generate expression to create a fresh symbolic variable."""
    if field.kind == "bool":
        return f'z3w::SymBool(ctx, {prefix_expr} + ".{field.name}")'
    elif field.kind == "bitvec" or field.kind == "enum_ref":
        stype = _symbolic_type(field)
        return f'{stype}(ctx, {prefix_expr} + ".{field.name}")'
    elif field.kind == "struct_ref":
        stype = _symbolic_type(field)
        return f'{stype}::Create(ctx, {prefix_expr} + ".{field.name}")'
    raise ValueError(f"Unknown kind: {field.kind}")


def _pack_expr(field: ResolvedField, accessor: str) -> str:
    """Generate expression to convert a symbolic field to Ubv for packing."""
    if field.kind == "bool":
        return f"z3w::as_uint1({accessor})"
    elif field.kind == "bitvec" or field.kind == "enum_ref":
        if field.signedness == "signed":
            return (
                f"z3w::unsafe_cast<z3w::SymUInt<{field.element_width}>>"
                f"({accessor})"
            )
        return accessor
    elif field.kind == "struct_ref":
        return f"{accessor}.Pack()"
    raise ValueError(f"Unknown kind: {field.kind}")


# ---------------------------------------------------------------------------
# Declaration emitters (struct bodies with method signatures only)
# ---------------------------------------------------------------------------


def _emit_concrete_decl(lines: list, struct: ResolvedStruct) -> None:
    """Emit concrete struct declaration with method signatures."""
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


def _emit_symbolic_decl(lines: list, struct: ResolvedStruct) -> None:
    """Emit symbolic struct declaration with method signatures."""
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
    lines.append("      const std::string& prefix);")
    lines.append(f"  z3w::SymUInt<{struct.total_width}> Pack() const;")
    lines.append(
        f"  {struct.name}Concrete ToConcrete(const z3::model& model) const;"
    )
    lines.append(
        f"  static {struct.name}Symbolic FromConcrete(z3::context& ctx,"
    )
    lines.append(f"      const {struct.name}Concrete& concrete);")
    lines.append("};")


# ---------------------------------------------------------------------------
# Implementation emitters (out-of-line inline method definitions)
# ---------------------------------------------------------------------------


def _emit_to_proto_impl(lines: list, struct: ResolvedStruct) -> None:
    """Generate out-of-line ToProto implementation."""
    lines.append(
        f"inline {struct.name}Proto {struct.name}Concrete::ToProto() const {{"
    )
    lines.append(f"  {struct.name}Proto proto;")
    for field in struct.fields:
        if field.reserved:
            continue
        if field.count >= 1:
            lines.append(f"  for (size_t i = 0; i < {field.count}; ++i) {{")
            expr = _to_proto_expr(field, f"{field.name}[i]")
            if field.kind == "struct_ref":
                lines.append(f"    *proto.add_{field.name}() = {expr};")
            else:
                lines.append(f"    proto.add_{field.name}({expr});")
            lines.append("  }")
        else:
            expr = _to_proto_expr(field, field.name)
            if field.kind == "struct_ref":
                lines.append(f"  *proto.mutable_{field.name}() = {expr};")
            else:
                lines.append(f"  proto.set_{field.name}({expr});")
    lines.append("  return proto;")
    lines.append("}")


def _emit_from_proto_impl(lines: list, struct: ResolvedStruct) -> None:
    """Generate out-of-line FromProto implementation."""
    lines.append(
        f"inline {struct.name}Concrete {struct.name}Concrete::FromProto("
        f"const {struct.name}Proto& proto) {{"
    )
    lines.append(f"  {struct.name}Concrete result{{}};")
    for field in struct.fields:
        if field.reserved:
            continue
        if field.count >= 1:
            lines.append(f"  for (size_t i = 0; i < {field.count}; ++i) {{")
            expr = _from_proto_expr(field, f"proto.{field.name}(i)")
            lines.append(f"    result.{field.name}[i] = {expr};")
            lines.append("  }")
        else:
            expr = _from_proto_expr(field, f"proto.{field.name}()")
            lines.append(f"  result.{field.name} = {expr};")
    lines.append("  return result;")
    lines.append("}")


def _emit_create_impl(lines: list, struct: ResolvedStruct) -> None:
    """Generate out-of-line Create implementation."""
    lines.append(
        f"inline {struct.name}Symbolic {struct.name}Symbolic::Create("
        f"z3::context& ctx,"
    )
    lines.append("    const std::string& prefix) {")
    lines.append(f"  {struct.name}Symbolic result;")
    for field in struct.fields:
        if field.count >= 1:
            lines.append(f"  for (size_t i = 0; i < {field.count}; ++i) {{")
            if field.kind == "bool":
                lines.append(
                    f"    result.{field.name}[i] = z3w::SymBool(ctx,"
                    f' prefix + ".{field.name}[" + std::to_string(i) + "]");'
                )
            elif field.kind == "struct_ref":
                stype = _symbolic_type(field)
                lines.append(
                    f"    result.{field.name}[i] = {stype}::Create(ctx,"
                    f' prefix + ".{field.name}[" + std::to_string(i) + "]");'
                )
            else:
                stype = _symbolic_type(field)
                lines.append(
                    f"    result.{field.name}[i] = {stype}(ctx,"
                    f' prefix + ".{field.name}[" + std::to_string(i) + "]");'
                )
            lines.append("  }")
        else:
            expr = _create_expr(field, "prefix")
            lines.append(f"  result.{field.name} = {expr};")
    lines.append("  return result;")
    lines.append("}")


def _emit_pack_impl(lines: list, struct: ResolvedStruct) -> None:
    """Generate out-of-line Pack implementation."""
    lines.append(
        f"inline z3w::SymUInt<{struct.total_width}> "
        f"{struct.name}Symbolic::Pack() const {{"
    )
    pack_parts = []
    for field in struct.fields:
        if field.count >= 1:
            for i in range(field.count):
                pack_parts.append(_pack_expr(field, f"{field.name}[{i}]"))
        else:
            pack_parts.append(_pack_expr(field, field.name))

    if len(pack_parts) == 1:
        lines.append(f"  return {pack_parts[0]};")
    else:
        # concat(high, low): first arg is the most significant bits.
        # LSB_FIRST: fields listed low-to-high, so reverse for concat.
        # MSB_FIRST: fields listed high-to-low, already correct order.
        if struct.field_pack_order == "lsb_first":
            ordered_parts = list(reversed(pack_parts))
        else:
            ordered_parts = pack_parts
        lines.append(f"  return z3w::concat({', '.join(ordered_parts)});")
    lines.append("}")


def _emit_to_concrete_impl(lines: list, struct: ResolvedStruct) -> None:
    """Generate out-of-line ToConcrete implementation."""
    lines.append(
        f"inline {struct.name}Concrete "
        f"{struct.name}Symbolic::ToConcrete(const z3::model& model) const {{"
    )
    lines.append(f"  {struct.name}Concrete result{{}};")
    for field in struct.fields:
        if field.count >= 1:
            lines.append(f"  for (size_t i = 0; i < {field.count}; ++i) {{")
            expr = _to_concrete_expr(field, f"{field.name}[i]")
            lines.append(f"    result.{field.name}[i] = {expr};")
            lines.append("  }")
        else:
            expr = _to_concrete_expr(field, field.name)
            lines.append(f"  result.{field.name} = {expr};")
    lines.append("  return result;")
    lines.append("}")


def _emit_from_concrete_impl(lines: list, struct: ResolvedStruct) -> None:
    """Generate out-of-line FromConcrete implementation."""
    lines.append(
        f"inline {struct.name}Symbolic "
        f"{struct.name}Symbolic::FromConcrete(z3::context& ctx,"
    )
    lines.append(f"    const {struct.name}Concrete& concrete) {{")
    lines.append(f"  {struct.name}Symbolic result;")
    for field in struct.fields:
        if field.count >= 1:
            lines.append(f"  for (size_t i = 0; i < {field.count}; ++i) {{")
            expr = _from_concrete_expr(field, f"concrete.{field.name}[i]")
            lines.append(f"    result.{field.name}[i] = {expr};")
            lines.append("  }")
        else:
            expr = _from_concrete_expr(field, f"concrete.{field.name}")
            lines.append(f"  result.{field.name} = {expr};")
    lines.append("  return result;")
    lines.append("}")


# ---------------------------------------------------------------------------
# Main emitter
# ---------------------------------------------------------------------------


_SECTION_SEPARATOR = "// " + "=" * 61


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
    lines.append("#include <tuple>")
    lines.append("")
    lines.append('#include "z3wire/bool.h"')
    lines.append('#include "z3wire/sym_bool.h"')
    lines.append('#include "z3wire/sym_bit_vec.h"')
    lines.append('#include "z3wire/bit_vec.h"')
    lines.append(f'#include "{proto_header}"')
    lines.append("")
    lines.append(f"namespace {module.namespace} {{")
    lines.append("")

    # ---- Enum constants ----
    if module.enums:
        lines.append(_SECTION_SEPARATOR)
        lines.append("// Enum constants")
        lines.append(_SECTION_SEPARATOR)
        lines.append("")

        for enum in module.enums:
            if enum.desc:
                lines.append(f"// {enum.desc} (width: {enum.width})")
            lines.append(f"struct {enum.name} {{")
            for name, _desc, value in enum.values:
                lit = f"z3w::UInt<{enum.width}>::Literal<{value}>()"
                lines.append(f"  static constexpr auto {name} = {lit};")
            lines.append("};")
            lines.append("")

    # ---- Concrete types ----
    lines.append(_SECTION_SEPARATOR)
    lines.append("// Concrete types")
    lines.append(_SECTION_SEPARATOR)
    lines.append("")

    for struct in module.structs:
        _emit_concrete_decl(lines, struct)
        lines.append("")

    # ---- Symbolic types ----
    lines.append(_SECTION_SEPARATOR)
    lines.append("// Symbolic types")
    lines.append(_SECTION_SEPARATOR)
    lines.append("")

    for struct in module.structs:
        _emit_symbolic_decl(lines, struct)
        lines.append("")

    # ---- Inline implementations ----
    lines.append(_SECTION_SEPARATOR)
    lines.append("// Inline implementations")
    lines.append(_SECTION_SEPARATOR)
    lines.append("")

    for struct in module.structs:
        _emit_to_proto_impl(lines, struct)
        lines.append("")
        _emit_from_proto_impl(lines, struct)
        lines.append("")
        _emit_create_impl(lines, struct)
        lines.append("")
        _emit_pack_impl(lines, struct)
        lines.append("")
        _emit_to_concrete_impl(lines, struct)
        lines.append("")
        _emit_from_concrete_impl(lines, struct)
        lines.append("")

    lines.append(f"}}  // namespace {module.namespace}")
    lines.append("")

    return "\n".join(lines)
