#include "z3wire/weave/emit_header.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "z3wire/weave/resolver.h"

namespace z3wire::weave {

namespace {

constexpr char kSectionSeparator[] =
    "// =============================================================";

std::string SymbolicType(const ResolvedField& field) {
  switch (field.kind) {
    case ResolvedField::kBool:
      return "z3w::SymBool";
    case ResolvedField::kBitVec:
    case ResolvedField::kEnumRef:
      if (field.signedness == ResolvedField::kSigned) {
        return absl::StrFormat("z3w::SymSInt<%d>", field.element_width);
      }
      return absl::StrFormat("z3w::SymUInt<%d>", field.element_width);
    case ResolvedField::kStructRef:
      return absl::StrCat(field.struct_name, "Symbolic");
    default:
      throw std::invalid_argument("Unknown field kind");
  }
}

std::string ConcreteType(const ResolvedField& field) {
  switch (field.kind) {
    case ResolvedField::kBool:
      return "z3w::Bool";
    case ResolvedField::kBitVec:
    case ResolvedField::kEnumRef:
      if (field.signedness == ResolvedField::kSigned) {
        return absl::StrFormat("z3w::SInt<%d>", field.element_width);
      }
      return absl::StrFormat("z3w::UInt<%d>", field.element_width);
    case ResolvedField::kStructRef:
      return absl::StrCat(field.struct_name, "Concrete");
    default:
      throw std::invalid_argument("Unknown field kind");
  }
}

std::string ProtoValueType(const ResolvedField& field) {
  switch (field.kind) {
    case ResolvedField::kBool:
      return "bool";
    case ResolvedField::kBitVec:
    case ResolvedField::kEnumRef:
      if (field.signedness == ResolvedField::kSigned) {
        return field.element_width > 32 ? "int64_t" : "int32_t";
      }
      return field.element_width > 32 ? "uint64_t" : "uint32_t";
    case ResolvedField::kStructRef:
      return absl::StrCat(field.struct_name, "Proto");
    default:
      throw std::invalid_argument("Unknown field kind");
  }
}

std::string FieldDecl(const std::string& type_str, const ResolvedField& field) {
  if (field.count >= 1) {
    return absl::StrFormat("std::array<%s, %d> %s;", type_str, field.count,
                           field.name);
  }
  return absl::StrCat(type_str, " ", field.name, ";");
}

std::string ToProtoExpr(const ResolvedField& field,
                        const std::string& accessor) {
  switch (field.kind) {
    case ResolvedField::kBool:
      return absl::StrFormat("bool(%s)", accessor);
    case ResolvedField::kBitVec:
    case ResolvedField::kEnumRef:
      return absl::StrFormat("static_cast<%s>(%s.value())",
                             ProtoValueType(field), accessor);
    case ResolvedField::kStructRef:
      return absl::StrCat(accessor, ".ToProto()");
    default:
      throw std::invalid_argument("Unknown field kind");
  }
}

std::string FromProtoExpr(const ResolvedField& field,
                          const std::string& accessor) {
  switch (field.kind) {
    case ResolvedField::kBool:
      return absl::StrFormat("z3w::Bool(%s)", accessor);
    case ResolvedField::kBitVec:
    case ResolvedField::kEnumRef: {
      std::string ctype = ConcreteType(field);
      return absl::StrFormat("std::get<0>(%s::Checked(%s))", ctype, accessor);
    }
    case ResolvedField::kStructRef: {
      std::string ctype = ConcreteType(field);
      return absl::StrFormat("%s::FromProto(%s)", ctype, accessor);
    }
    default:
      throw std::invalid_argument("Unknown field kind");
  }
}

std::string ToConcreteExpr(const ResolvedField& field,
                           const std::string& accessor) {
  switch (field.kind) {
    case ResolvedField::kBool:
      return absl::StrFormat(
          "z3w::Bool(model.eval(%s.expr(), true).is_true())", accessor);
    case ResolvedField::kBitVec:
    case ResolvedField::kEnumRef: {
      std::string ctype = ConcreteType(field);
      std::string pvtype = ProtoValueType(field);
      return absl::StrFormat(
          "std::get<0>(%s::Checked(static_cast<%s>("
          "model.eval(%s.expr(), true).get_numeral_int64())))",
          ctype, pvtype, accessor);
    }
    case ResolvedField::kStructRef:
      return absl::StrCat(accessor, ".ToConcrete(model)");
    default:
      throw std::invalid_argument("Unknown field kind");
  }
}

std::string FromConcreteExpr(const ResolvedField& field,
                             const std::string& accessor) {
  switch (field.kind) {
    case ResolvedField::kBool:
      return absl::StrFormat("z3w::to_symbolic(%s, ctx)", accessor);
    case ResolvedField::kBitVec:
    case ResolvedField::kEnumRef:
      return absl::StrFormat("z3w::to_symbolic(%s, ctx)", accessor);
    case ResolvedField::kStructRef: {
      std::string stype = SymbolicType(field);
      return absl::StrFormat("%s::FromConcrete(ctx, %s)", stype, accessor);
    }
    default:
      throw std::invalid_argument("Unknown field kind");
  }
}

std::string CreateExpr(const ResolvedField& field,
                       const std::string& prefix_expr) {
  switch (field.kind) {
    case ResolvedField::kBool:
      return absl::StrFormat("z3w::SymBool(ctx, %s + \".%s\")", prefix_expr,
                             field.name);
    case ResolvedField::kBitVec:
    case ResolvedField::kEnumRef: {
      std::string stype = SymbolicType(field);
      return absl::StrFormat("%s(ctx, %s + \".%s\")", stype, prefix_expr,
                             field.name);
    }
    case ResolvedField::kStructRef: {
      std::string stype = SymbolicType(field);
      return absl::StrFormat("%s::Create(ctx, %s + \".%s\")", stype,
                             prefix_expr, field.name);
    }
    default:
      throw std::invalid_argument("Unknown field kind");
  }
}

std::string PackExpr(const ResolvedField& field,
                     const std::string& accessor) {
  switch (field.kind) {
    case ResolvedField::kBool:
      return absl::StrFormat("z3w::as_uint1(%s)", accessor);
    case ResolvedField::kBitVec:
    case ResolvedField::kEnumRef:
      if (field.signedness == ResolvedField::kSigned) {
        return absl::StrFormat(
            "z3w::unsafe_cast<z3w::SymUInt<%d>>(%s)", field.element_width,
            accessor);
      }
      return accessor;
    case ResolvedField::kStructRef:
      return absl::StrCat(accessor, ".Pack()");
    default:
      throw std::invalid_argument("Unknown field kind");
  }
}

// ---------------------------------------------------------------------------
// Declaration emitters
// ---------------------------------------------------------------------------

void EmitConcreteDecl(std::string& out, const ResolvedStruct& s) {
  if (!s.desc.empty()) {
    absl::StrAppend(&out, "// ", s.desc, "\n");
  }
  absl::StrAppend(&out, "struct ", s.name, "Concrete {\n");
  for (const auto& f : s.fields) {
    absl::StrAppend(&out, "  ", FieldDecl(ConcreteType(f), f), "\n");
  }
  absl::StrAppend(&out, "\n");
  absl::StrAppend(&out, "  ", s.name, "Proto ToProto() const;\n");
  absl::StrAppend(&out, "  static ", s.name, "Concrete FromProto(const ",
                   s.name, "Proto& proto);\n");
  absl::StrAppend(&out, "};\n");
}

void EmitSymbolicDecl(std::string& out, const ResolvedStruct& s) {
  if (!s.desc.empty()) {
    absl::StrAppend(&out, "// ", s.desc, " (symbolic)\n");
  }
  std::string pack_label =
      s.field_pack_order == ResolvedStruct::kLsbFirst ? "LSB" : "MSB";
  absl::StrAppend(&out, "// Total width: ", s.total_width,
                   " bits, field pack order: ", pack_label, " first\n");
  absl::StrAppend(&out, "struct ", s.name, "Symbolic {\n");
  for (const auto& f : s.fields) {
    std::string decl = FieldDecl(SymbolicType(f), f);
    int high = f.offset + f.width - 1;
    std::string bit_comment;
    if (f.width == 1) {
      bit_comment = absl::StrFormat("  // [%d]", f.offset);
    } else {
      bit_comment = absl::StrFormat("  // [%d:%d]", high, f.offset);
    }
    absl::StrAppend(&out, "  ", decl, bit_comment, "\n");
  }
  absl::StrAppend(&out, "\n");
  absl::StrAppend(&out, "  static ", s.name,
                   "Symbolic Create(z3::context& ctx,\n");
  absl::StrAppend(&out, "      const std::string& prefix);\n");
  absl::StrAppend(&out, "  z3w::SymUInt<", s.total_width,
                   "> Pack() const;\n");
  absl::StrAppend(&out, "  ", s.name,
                   "Concrete ToConcrete(const z3::model& model) const;\n");
  absl::StrAppend(&out, "  static ", s.name,
                   "Symbolic FromConcrete(z3::context& ctx,\n");
  absl::StrAppend(&out, "      const ", s.name, "Concrete& concrete);\n");
  absl::StrAppend(&out, "};\n");
}

// ---------------------------------------------------------------------------
// Implementation emitters
// ---------------------------------------------------------------------------

void EmitToProtoImpl(std::string& out, const ResolvedStruct& s) {
  absl::StrAppend(&out, "inline ", s.name, "Proto ", s.name,
                   "Concrete::ToProto() const {\n");
  absl::StrAppend(&out, "  ", s.name, "Proto proto;\n");
  for (const auto& f : s.fields) {
    if (f.reserved) continue;
    if (f.count >= 1) {
      absl::StrAppend(&out, "  for (size_t i = 0; i < ", f.count,
                       "; ++i) {\n");
      std::string expr =
          ToProtoExpr(f, absl::StrCat(f.name, "[i]"));
      if (f.kind == ResolvedField::kStructRef) {
        absl::StrAppend(&out, "    *proto.add_", f.name, "() = ", expr,
                         ";\n");
      } else {
        absl::StrAppend(&out, "    proto.add_", f.name, "(", expr, ");\n");
      }
      absl::StrAppend(&out, "  }\n");
    } else {
      std::string expr = ToProtoExpr(f, f.name);
      if (f.kind == ResolvedField::kStructRef) {
        absl::StrAppend(&out, "  *proto.mutable_", f.name, "() = ", expr,
                         ";\n");
      } else {
        absl::StrAppend(&out, "  proto.set_", f.name, "(", expr, ");\n");
      }
    }
  }
  absl::StrAppend(&out, "  return proto;\n");
  absl::StrAppend(&out, "}\n");
}

void EmitFromProtoImpl(std::string& out, const ResolvedStruct& s) {
  absl::StrAppend(&out, "inline ", s.name, "Concrete ", s.name,
                   "Concrete::FromProto(const ", s.name, "Proto& proto) {\n");
  absl::StrAppend(&out, "  ", s.name, "Concrete result{};\n");
  for (const auto& f : s.fields) {
    if (f.reserved) continue;
    if (f.count >= 1) {
      absl::StrAppend(&out, "  for (size_t i = 0; i < ", f.count,
                       "; ++i) {\n");
      std::string expr = FromProtoExpr(
          f, absl::StrFormat("proto.%s(i)", f.name));
      absl::StrAppend(&out, "    result.", f.name, "[i] = ", expr, ";\n");
      absl::StrAppend(&out, "  }\n");
    } else {
      std::string expr = FromProtoExpr(
          f, absl::StrFormat("proto.%s()", f.name));
      absl::StrAppend(&out, "  result.", f.name, " = ", expr, ";\n");
    }
  }
  absl::StrAppend(&out, "  return result;\n");
  absl::StrAppend(&out, "}\n");
}

void EmitCreateImpl(std::string& out, const ResolvedStruct& s) {
  absl::StrAppend(&out, "inline ", s.name, "Symbolic ", s.name,
                   "Symbolic::Create(z3::context& ctx,\n");
  absl::StrAppend(&out, "    const std::string& prefix) {\n");
  absl::StrAppend(&out, "  ", s.name, "Symbolic result;\n");
  for (const auto& f : s.fields) {
    if (f.count >= 1) {
      absl::StrAppend(&out, "  for (size_t i = 0; i < ", f.count,
                       "; ++i) {\n");
      if (f.kind == ResolvedField::kBool) {
        absl::StrAppend(&out, "    result.", f.name,
                         "[i] = z3w::SymBool(ctx, prefix + \".", f.name,
                         "[\" + std::to_string(i) + \"]\");\n");
      } else if (f.kind == ResolvedField::kStructRef) {
        std::string stype = SymbolicType(f);
        absl::StrAppend(&out, "    result.", f.name, "[i] = ", stype,
                         "::Create(ctx, prefix + \".", f.name,
                         "[\" + std::to_string(i) + \"]\");\n");
      } else {
        std::string stype = SymbolicType(f);
        absl::StrAppend(&out, "    result.", f.name, "[i] = ", stype,
                         "(ctx, prefix + \".", f.name,
                         "[\" + std::to_string(i) + \"]\");\n");
      }
      absl::StrAppend(&out, "  }\n");
    } else {
      std::string expr = CreateExpr(f, "prefix");
      absl::StrAppend(&out, "  result.", f.name, " = ", expr, ";\n");
    }
  }
  absl::StrAppend(&out, "  return result;\n");
  absl::StrAppend(&out, "}\n");
}

void EmitPackImpl(std::string& out, const ResolvedStruct& s) {
  absl::StrAppend(&out, "inline z3w::SymUInt<", s.total_width, "> ", s.name,
                   "Symbolic::Pack() const {\n");
  std::vector<std::string> pack_parts;
  for (const auto& f : s.fields) {
    if (f.count >= 1) {
      for (int i = 0; i < f.count; ++i) {
        pack_parts.push_back(
            PackExpr(f, absl::StrFormat("%s[%d]", f.name, i)));
      }
    } else {
      pack_parts.push_back(PackExpr(f, f.name));
    }
  }

  if (pack_parts.size() == 1) {
    absl::StrAppend(&out, "  return ", pack_parts[0], ";\n");
  } else {
    // concat(high, low): first arg is the most significant bits.
    // LSB_FIRST: fields listed low-to-high, so reverse for concat.
    // MSB_FIRST: fields listed high-to-low, already correct order.
    std::vector<std::string> ordered_parts;
    if (s.field_pack_order == ResolvedStruct::kLsbFirst) {
      ordered_parts.assign(pack_parts.rbegin(), pack_parts.rend());
    } else {
      ordered_parts = pack_parts;
    }
    absl::StrAppend(&out, "  return z3w::concat(",
                     absl::StrJoin(ordered_parts, ", "), ");\n");
  }
  absl::StrAppend(&out, "}\n");
}

void EmitToConcreteImpl(std::string& out, const ResolvedStruct& s) {
  absl::StrAppend(&out, "inline ", s.name, "Concrete ", s.name,
                   "Symbolic::ToConcrete(const z3::model& model) const {\n");
  absl::StrAppend(&out, "  ", s.name, "Concrete result{};\n");
  for (const auto& f : s.fields) {
    if (f.count >= 1) {
      absl::StrAppend(&out, "  for (size_t i = 0; i < ", f.count,
                       "; ++i) {\n");
      std::string expr =
          ToConcreteExpr(f, absl::StrCat(f.name, "[i]"));
      absl::StrAppend(&out, "    result.", f.name, "[i] = ", expr, ";\n");
      absl::StrAppend(&out, "  }\n");
    } else {
      std::string expr = ToConcreteExpr(f, f.name);
      absl::StrAppend(&out, "  result.", f.name, " = ", expr, ";\n");
    }
  }
  absl::StrAppend(&out, "  return result;\n");
  absl::StrAppend(&out, "}\n");
}

void EmitFromConcreteImpl(std::string& out, const ResolvedStruct& s) {
  absl::StrAppend(&out, "inline ", s.name, "Symbolic ", s.name,
                   "Symbolic::FromConcrete(z3::context& ctx,\n");
  absl::StrAppend(&out, "    const ", s.name, "Concrete& concrete) {\n");
  absl::StrAppend(&out, "  ", s.name, "Symbolic result;\n");
  for (const auto& f : s.fields) {
    if (f.count >= 1) {
      absl::StrAppend(&out, "  for (size_t i = 0; i < ", f.count,
                       "; ++i) {\n");
      std::string expr = FromConcreteExpr(
          f, absl::StrFormat("concrete.%s[i]", f.name));
      absl::StrAppend(&out, "    result.", f.name, "[i] = ", expr, ";\n");
      absl::StrAppend(&out, "  }\n");
    } else {
      std::string expr = FromConcreteExpr(
          f, absl::StrFormat("concrete.%s", f.name));
      absl::StrAppend(&out, "  result.", f.name, " = ", expr, ";\n");
    }
  }
  absl::StrAppend(&out, "  return result;\n");
  absl::StrAppend(&out, "}\n");
}

}  // namespace

std::string EmitHeader(const ResolvedModule& module,
                       const std::string& proto_header) {
  std::string out;

  // Includes
  absl::StrAppend(&out, "#pragma once\n");
  absl::StrAppend(&out, "\n");

  bool has_arrays = false;
  for (const auto& s : module.structs) {
    for (const auto& f : s.fields) {
      if (f.count >= 1) {
        has_arrays = true;
        break;
      }
    }
    if (has_arrays) break;
  }
  if (has_arrays) {
    absl::StrAppend(&out, "#include <array>\n");
  }
  absl::StrAppend(&out, "#include <string>\n");
  absl::StrAppend(&out, "#include <tuple>\n");
  absl::StrAppend(&out, "\n");
  absl::StrAppend(&out, "#include \"z3wire/bool.h\"\n");
  absl::StrAppend(&out, "#include \"z3wire/sym_bool.h\"\n");
  absl::StrAppend(&out, "#include \"z3wire/sym_bit_vec.h\"\n");
  absl::StrAppend(&out, "#include \"z3wire/bit_vec.h\"\n");
  absl::StrAppend(&out, "#include \"", proto_header, "\"\n");
  absl::StrAppend(&out, "\n");
  absl::StrAppend(&out, "namespace ", module.ns, " {\n");
  absl::StrAppend(&out, "\n");

  // Enum constants
  if (!module.enums.empty()) {
    absl::StrAppend(&out, kSectionSeparator, "\n");
    absl::StrAppend(&out, "// Enum constants\n");
    absl::StrAppend(&out, kSectionSeparator, "\n");
    absl::StrAppend(&out, "\n");

    for (const auto& e : module.enums) {
      if (!e.desc.empty()) {
        absl::StrAppend(&out, "// ", e.desc, " (width: ", e.width, ")\n");
      }
      absl::StrAppend(&out, "struct ", e.name, " {\n");
      for (const auto& v : e.values) {
        absl::StrAppend(&out, "  static constexpr auto ", v.name,
                         " = z3w::UInt<", e.width, ">::Literal<", v.value,
                         ">();\n");
      }
      absl::StrAppend(&out, "};\n");
      absl::StrAppend(&out, "\n");
    }
  }

  // Concrete types
  absl::StrAppend(&out, kSectionSeparator, "\n");
  absl::StrAppend(&out, "// Concrete types\n");
  absl::StrAppend(&out, kSectionSeparator, "\n");
  absl::StrAppend(&out, "\n");

  for (const auto& s : module.structs) {
    EmitConcreteDecl(out, s);
    absl::StrAppend(&out, "\n");
  }

  // Symbolic types
  absl::StrAppend(&out, kSectionSeparator, "\n");
  absl::StrAppend(&out, "// Symbolic types\n");
  absl::StrAppend(&out, kSectionSeparator, "\n");
  absl::StrAppend(&out, "\n");

  for (const auto& s : module.structs) {
    EmitSymbolicDecl(out, s);
    absl::StrAppend(&out, "\n");
  }

  // Inline implementations
  absl::StrAppend(&out, kSectionSeparator, "\n");
  absl::StrAppend(&out, "// Inline implementations\n");
  absl::StrAppend(&out, kSectionSeparator, "\n");
  absl::StrAppend(&out, "\n");

  for (const auto& s : module.structs) {
    EmitToProtoImpl(out, s);
    absl::StrAppend(&out, "\n");
    EmitFromProtoImpl(out, s);
    absl::StrAppend(&out, "\n");
    EmitCreateImpl(out, s);
    absl::StrAppend(&out, "\n");
    EmitPackImpl(out, s);
    absl::StrAppend(&out, "\n");
    EmitToConcreteImpl(out, s);
    absl::StrAppend(&out, "\n");
    EmitFromConcreteImpl(out, s);
    absl::StrAppend(&out, "\n");
  }

  absl::StrAppend(&out, "}  // namespace ", module.ns, "\n");

  return out;
}

}  // namespace z3wire::weave
