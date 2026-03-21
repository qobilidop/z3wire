#include "z3wire_weave/emit_proto.h"

#include <stdexcept>
#include <string>

#include "absl/strings/str_cat.h"
#include "z3wire_weave/resolver.h"

namespace z3wire_weave {

namespace {

std::string ProtoType(const ResolvedField& field) {
  switch (field.kind) {
    case ResolvedField::kBool:
      return "bool";
    case ResolvedField::kBitVec:
      if (field.signedness == ResolvedField::kSigned) {
        return field.element_width > 32 ? "sint64" : "sint32";
      }
      return field.element_width > 32 ? "uint64" : "uint32";
    case ResolvedField::kEnumRef:
      return field.element_width > 32 ? "uint64" : "uint32";
    case ResolvedField::kStructRef:
      return field.struct_name + "Proto";
    default:
      throw std::invalid_argument("Unknown field kind");
  }
}

}  // namespace

std::string EmitProto(const ResolvedModule& module) {
  std::string out;
  absl::StrAppend(&out, "syntax = \"proto3\";\n");
  absl::StrAppend(&out, "package ", module.ns, ";\n");
  absl::StrAppend(&out, "\n");

  for (const auto& s : module.structs) {
    if (!s.desc.empty()) {
      absl::StrAppend(&out, "// ", s.desc, "\n");
    }
    absl::StrAppend(&out, "message ", s.name, "Proto {\n");
    int field_number = 1;
    for (const auto& f : s.fields) {
      if (f.reserved) {
        continue;
      }
      std::string proto_type = ProtoType(f);
      if (f.count >= 1) {
        absl::StrAppend(&out, "  repeated ", proto_type, " ", f.name, " = ",
                        field_number, ";\n");
      } else {
        absl::StrAppend(&out, "  ", proto_type, " ", f.name, " = ",
                        field_number, ";\n");
      }
      ++field_number;
    }
    absl::StrAppend(&out, "}\n");
    absl::StrAppend(&out, "\n");
  }

  // Remove trailing blank line to match Python output format.
  if (!out.empty() && out.back() == '\n' && out.size() >= 2 &&
      out[out.size() - 2] == '\n') {
    out.pop_back();
  }

  return out;
}

}  // namespace z3wire_weave
