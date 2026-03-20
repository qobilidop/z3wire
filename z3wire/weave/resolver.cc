#include "z3wire/weave/resolver.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/str_format.h"
#include "z3wire/weave/rdl.pb.h"

namespace z3wire::weave {
namespace {

const std::set<std::string> kReservedNames = {
    "Create", "Pack", "ToConcrete", "FromConcrete", "ToProto", "FromProto",
};

std::vector<ResolvedEnum> ResolveEnums(const z3wire_rdl::Module& module) {
  std::vector<ResolvedEnum> resolved;
  for (const auto& enum_def : module.enums()) {
    uint64_t max_val = (1ULL << enum_def.width()) - 1;
    for (const auto& v : enum_def.values()) {
      if (v.value() > max_val) {
        throw std::invalid_argument(absl::StrFormat(
            "Enum '%s': value '%s' = %d exceeds width %d (max %d)",
            enum_def.name(), v.name(), v.value(), enum_def.width(), max_val));
      }
    }
    std::set<uint64_t> seen_values;
    for (const auto& v : enum_def.values()) {
      if (seen_values.count(v.value())) {
        throw std::invalid_argument(absl::StrFormat(
            "Enum '%s': duplicate value %d", enum_def.name(), v.value()));
      }
      seen_values.insert(v.value());
    }
    ResolvedEnum re;
    re.name = enum_def.name();
    re.desc = enum_def.desc();
    re.width = static_cast<int>(enum_def.width());
    for (const auto& v : enum_def.values()) {
      re.values.push_back({v.name(), v.desc(), v.value()});
    }
    resolved.push_back(std::move(re));
  }
  return resolved;
}

int FieldElementWidth(
    const z3wire_rdl::FieldType& field_type,
    const std::unordered_map<std::string, ResolvedEnum>& enum_map,
    const std::unordered_map<std::string, ResolvedStruct>& struct_map) {
  switch (field_type.kind_case()) {
    case z3wire_rdl::FieldType::kBool:
      return 1;
    case z3wire_rdl::FieldType::kBitvec:
      return static_cast<int>(field_type.bitvec().width());
    case z3wire_rdl::FieldType::kEnumRef: {
      auto it = enum_map.find(field_type.enum_ref());
      if (it == enum_map.end()) {
        throw std::invalid_argument(
            absl::StrFormat("Unknown enum_ref '%s'", field_type.enum_ref()));
      }
      return it->second.width;
    }
    case z3wire_rdl::FieldType::kStructRef: {
      auto it = struct_map.find(field_type.struct_ref());
      return it->second.total_width;
    }
    default:
      throw std::invalid_argument("Unknown field type");
  }
}

ResolvedStruct::PackOrder ResolvePackOrder(
    const z3wire_rdl::Module& module, const z3wire_rdl::Struct& struct_def) {
  auto order = struct_def.field_pack_order();
  if (order == z3wire_rdl::FIELD_PACK_ORDER_UNSPECIFIED) {
    order = module.field_pack_order();
  }
  if (order == z3wire_rdl::FIELD_PACK_ORDER_UNSPECIFIED) {
    throw std::invalid_argument(absl::StrFormat(
        "Struct '%s': field_pack_order not specified at struct or module level",
        struct_def.name()));
  }
  if (order == z3wire_rdl::FIELD_PACK_ORDER_LSB_FIRST) {
    return ResolvedStruct::kLsbFirst;
  }
  return ResolvedStruct::kMsbFirst;
}

std::vector<ResolvedStruct> ResolveStructs(
    const z3wire_rdl::Module& module,
    const std::unordered_map<std::string, ResolvedEnum>& enum_map) {
  std::unordered_map<std::string, ResolvedStruct> struct_map;
  std::vector<ResolvedStruct> resolved;

  // Topological sort via retry loop.
  std::vector<z3wire_rdl::Struct> pending(module.structs().begin(),
                                          module.structs().end());
  int max_iterations =
      static_cast<int>(pending.size()) * static_cast<int>(pending.size()) + 1;
  int iteration = 0;

  while (!pending.empty()) {
    ++iteration;
    if (iteration > max_iterations) {
      std::string names;
      for (const auto& s : pending) {
        if (!names.empty()) names += ", ";
        names += "'" + s.name() + "'";
      }
      throw std::invalid_argument(
          absl::StrFormat("Circular struct dependency: [%s]", names));
    }

    auto struct_def = std::move(pending.front());
    pending.erase(pending.begin());

    // Check if all struct_ref dependencies are resolved.
    bool deps_met = true;
    for (const auto& f : struct_def.fields()) {
      if (f.type().kind_case() == z3wire_rdl::FieldType::kStructRef &&
          struct_map.find(f.type().struct_ref()) == struct_map.end()) {
        deps_met = false;
        break;
      }
    }
    if (!deps_met) {
      pending.push_back(std::move(struct_def));
      continue;
    }

    auto pack_order = ResolvePackOrder(module, struct_def);

    // Validate field names.
    for (const auto& f : struct_def.fields()) {
      if (kReservedNames.count(f.name())) {
        throw std::invalid_argument(
            absl::StrFormat("Struct '%s': field name '%s' is reserved "
                            "(collides with generated method)",
                            struct_def.name(), f.name()));
      }
      if (f.type().kind_case() == z3wire_rdl::FieldType::kEnumRef &&
          enum_map.find(f.type().enum_ref()) == enum_map.end()) {
        throw std::invalid_argument(
            absl::StrFormat("Struct '%s', field '%s': unknown enum_ref '%s'",
                            struct_def.name(), f.name(), f.type().enum_ref()));
      }
    }

    // First pass: compute field widths.
    struct FieldInfo {
      const z3wire_rdl::Field* field;
      ResolvedField::Kind kind;
      int element_width;
      int count;
      int total_width;
      ResolvedField::Signedness signedness;
    };
    std::vector<FieldInfo> field_infos;

    for (const auto& f : struct_def.fields()) {
      int element_width = FieldElementWidth(f.type(), enum_map, struct_map);
      int count = static_cast<int>(f.count());
      int total_width = element_width * std::max(count, 1);

      ResolvedField::Kind kind;
      switch (f.type().kind_case()) {
        case z3wire_rdl::FieldType::kBool:
          kind = ResolvedField::kBool;
          break;
        case z3wire_rdl::FieldType::kBitvec:
          kind = ResolvedField::kBitVec;
          break;
        case z3wire_rdl::FieldType::kEnumRef:
          kind = ResolvedField::kEnumRef;
          break;
        case z3wire_rdl::FieldType::kStructRef:
          kind = ResolvedField::kStructRef;
          break;
        default:
          kind = ResolvedField::kBitVec;
          break;
      }

      auto signedness = ResolvedField::kUnsigned;
      if (f.type().kind_case() == z3wire_rdl::FieldType::kBitvec &&
          f.type().bitvec().signedness() == z3wire_rdl::SIGNEDNESS_SIGNED) {
        signedness = ResolvedField::kSigned;
      }

      field_infos.push_back(
          {&f, kind, element_width, count, total_width, signedness});
    }

    int struct_total_width = 0;
    for (const auto& fi : field_infos) {
      struct_total_width += fi.total_width;
    }

    // Second pass: assign offsets based on pack order.
    std::vector<ResolvedField> fields;
    if (pack_order == ResolvedStruct::kLsbFirst) {
      int offset = 0;
      for (const auto& fi : field_infos) {
        ResolvedField rf;
        rf.name = fi.field->name();
        rf.desc = fi.field->desc();
        rf.width = fi.total_width;
        rf.offset = offset;
        rf.count = fi.count;
        rf.element_width = fi.element_width;
        rf.reserved = fi.field->reserved();
        rf.kind = fi.kind;
        rf.signedness = fi.signedness;
        if (fi.kind == ResolvedField::kEnumRef) {
          rf.enum_name = fi.field->type().enum_ref();
        }
        if (fi.kind == ResolvedField::kStructRef) {
          rf.struct_name = fi.field->type().struct_ref();
        }
        fields.push_back(std::move(rf));
        offset += fi.total_width;
      }
    } else {
      // MSB_FIRST: first field in list occupies highest bits.
      int offset = struct_total_width;
      for (const auto& fi : field_infos) {
        offset -= fi.total_width;
        ResolvedField rf;
        rf.name = fi.field->name();
        rf.desc = fi.field->desc();
        rf.width = fi.total_width;
        rf.offset = offset;
        rf.count = fi.count;
        rf.element_width = fi.element_width;
        rf.reserved = fi.field->reserved();
        rf.kind = fi.kind;
        rf.signedness = fi.signedness;
        if (fi.kind == ResolvedField::kEnumRef) {
          rf.enum_name = fi.field->type().enum_ref();
        }
        if (fi.kind == ResolvedField::kStructRef) {
          rf.struct_name = fi.field->type().struct_ref();
        }
        fields.push_back(std::move(rf));
      }
    }

    if (struct_def.has_width() &&
        static_cast<int>(struct_def.width()) != struct_total_width) {
      throw std::invalid_argument(absl::StrFormat(
          "Struct '%s': declared width %d but fields sum to %d",
          struct_def.name(), struct_def.width(), struct_total_width));
    }

    ResolvedStruct rs;
    rs.name = struct_def.name();
    rs.desc = struct_def.desc();
    rs.total_width = struct_total_width;
    rs.field_pack_order = pack_order;
    rs.fields = std::move(fields);
    struct_map[rs.name] = rs;
    resolved.push_back(std::move(rs));
  }

  return resolved;
}

}  // namespace

ResolvedModule Resolve(const z3wire_rdl::Module& module) {
  if (module.file_prefix().empty()) {
    throw std::invalid_argument("Module must specify file_prefix");
  }
  auto enums = ResolveEnums(module);
  std::unordered_map<std::string, ResolvedEnum> enum_map;
  for (const auto& e : enums) {
    enum_map[e.name] = e;
  }
  auto structs = ResolveStructs(module, enum_map);
  return ResolvedModule{
      .file_prefix = module.file_prefix(),
      .ns = module.namespace_(),
      .enums = std::move(enums),
      .structs = std::move(structs),
  };
}

}  // namespace z3wire::weave
