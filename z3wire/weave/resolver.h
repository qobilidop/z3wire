#ifndef Z3WIRE_WEAVE_RESOLVER_H_
#define Z3WIRE_WEAVE_RESOLVER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "z3wire/weave/rdl.pb.h"

namespace z3wire::weave {

struct ResolvedField {
  std::string name;
  std::string desc;
  int width;           // Total width in bits (element_width * max(count, 1))
  int offset;          // Bit offset within parent struct
  int count;           // 0 = scalar, >= 1 = array
  int element_width;   // Width of a single element
  bool reserved;
  enum Kind { kBool, kBitVec, kEnumRef, kStructRef };
  Kind kind;
  enum Signedness { kUnsigned, kSigned };
  Signedness signedness = kUnsigned;
  std::string enum_name;
  std::string struct_name;
};

struct ResolvedEnum {
  std::string name;
  std::string desc;
  int width;
  struct Value {
    std::string name;
    std::string desc;
    uint64_t value;
  };
  std::vector<Value> values;
};

struct ResolvedStruct {
  std::string name;
  std::string desc;
  int total_width;
  enum PackOrder { kLsbFirst, kMsbFirst };
  PackOrder field_pack_order;
  std::vector<ResolvedField> fields;
};

struct ResolvedModule {
  std::string file_prefix;
  std::string ns;  // "namespace" is a keyword
  std::vector<ResolvedEnum> enums;
  std::vector<ResolvedStruct> structs;
};

// Resolves an RDL module: validates and computes layouts.
// Throws std::invalid_argument on validation errors.
ResolvedModule Resolve(const z3wire_rdl::Module& module);

}  // namespace z3wire::weave

#endif  // Z3WIRE_WEAVE_RESOLVER_H_
