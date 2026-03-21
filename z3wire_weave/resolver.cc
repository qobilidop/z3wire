#include "z3wire_weave/resolver.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "z3wire_weave/rdl.pb.h"

namespace z3wire_weave {
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
      if (seen_values.contains(v.value())) {
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

struct FieldInfo {
  const z3wire_rdl::Field* field;
  ResolvedField::Kind kind;
  int element_width;
  int count;
  int total_width;
  ResolvedField::Signedness signedness;
};

bool AllDepsResolved(
    const z3wire_rdl::Struct& struct_def,
    const std::unordered_map<std::string, ResolvedStruct>& struct_map) {
  return std::ranges::all_of(struct_def.fields(), [&](const auto& f) {
    return f.type().kind_case() != z3wire_rdl::FieldType::kStructRef ||
           struct_map.contains(f.type().struct_ref());
  });
}

void ValidateFieldNames(
    const z3wire_rdl::Struct& struct_def,
    const std::unordered_map<std::string, ResolvedEnum>& enum_map) {
  for (const auto& f : struct_def.fields()) {
    if (kReservedNames.contains(f.name())) {
      throw std::invalid_argument(
          absl::StrFormat("Struct '%s': field name '%s' is reserved "
                          "(collides with generated method)",
                          struct_def.name(), f.name()));
    }
    if (f.type().kind_case() == z3wire_rdl::FieldType::kEnumRef &&
        !enum_map.contains(f.type().enum_ref())) {
      throw std::invalid_argument(
          absl::StrFormat("Struct '%s', field '%s': unknown enum_ref '%s'",
                          struct_def.name(), f.name(), f.type().enum_ref()));
    }
  }
}

ResolvedField::Kind ResolveFieldKind(
    z3wire_rdl::FieldType::KindCase kind_case) {
  switch (kind_case) {
    case z3wire_rdl::FieldType::kBool:
      return ResolvedField::kBool;
    case z3wire_rdl::FieldType::kBitvec:
      return ResolvedField::kBitVec;
    case z3wire_rdl::FieldType::kEnumRef:
      return ResolvedField::kEnumRef;
    case z3wire_rdl::FieldType::kStructRef:
      return ResolvedField::kStructRef;
    default:
      return ResolvedField::kBitVec;
  }
}

std::vector<FieldInfo> ComputeFieldInfos(
    const z3wire_rdl::Struct& struct_def,
    const std::unordered_map<std::string, ResolvedEnum>& enum_map,
    const std::unordered_map<std::string, ResolvedStruct>& struct_map) {
  std::vector<FieldInfo> field_infos;
  for (const auto& f : struct_def.fields()) {
    int element_width = FieldElementWidth(f.type(), enum_map, struct_map);
    int count = static_cast<int>(f.count());
    int total_width = element_width * std::max(count, 1);
    auto kind = ResolveFieldKind(f.type().kind_case());

    auto signedness = ResolvedField::kUnsigned;
    if (f.type().kind_case() == z3wire_rdl::FieldType::kBitvec &&
        f.type().bitvec().signedness() == z3wire_rdl::SIGNEDNESS_SIGNED) {
      signedness = ResolvedField::kSigned;
    }

    field_infos.push_back(
        {&f, kind, element_width, count, total_width, signedness});
  }
  return field_infos;
}

ResolvedField MakeResolvedField(const FieldInfo& fi, int offset) {
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
  return rf;
}

std::vector<ResolvedField> AssignOffsets(
    const std::vector<FieldInfo>& field_infos,
    ResolvedStruct::PackOrder pack_order, int struct_total_width) {
  std::vector<ResolvedField> fields;
  if (pack_order == ResolvedStruct::kLsbFirst) {
    int offset = 0;
    for (const auto& fi : field_infos) {
      fields.push_back(MakeResolvedField(fi, offset));
      offset += fi.total_width;
    }
  } else {
    int offset = struct_total_width;
    for (const auto& fi : field_infos) {
      offset -= fi.total_width;
      fields.push_back(MakeResolvedField(fi, offset));
    }
  }
  return fields;
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
        if (!names.empty()) {
          names += ", ";
        }
        names += "'" + s.name() + "'";
      }
      throw std::invalid_argument(
          absl::StrFormat("Circular struct dependency: [%s]", names));
    }

    auto struct_def = std::move(pending.front());
    pending.erase(pending.begin());

    if (!AllDepsResolved(struct_def, struct_map)) {
      pending.push_back(std::move(struct_def));
      continue;
    }

    auto pack_order = ResolvePackOrder(module, struct_def);
    ValidateFieldNames(struct_def, enum_map);
    auto field_infos = ComputeFieldInfos(struct_def, enum_map, struct_map);

    int struct_total_width = 0;
    for (const auto& fi : field_infos) {
      struct_total_width += fi.total_width;
    }

    auto fields = AssignOffsets(field_infos, pack_order, struct_total_width);

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

}  // namespace z3wire_weave
