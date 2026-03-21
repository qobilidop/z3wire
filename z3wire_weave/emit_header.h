#ifndef Z3WIRE_WEAVE_EMIT_HEADER_H_
#define Z3WIRE_WEAVE_EMIT_HEADER_H_

#include <string>

#include "z3wire_weave/resolver.h"

namespace z3wire_weave {

std::string EmitHeader(const ResolvedModule& module,
                       const std::string& proto_header);

}  // namespace z3wire_weave

#endif  // Z3WIRE_WEAVE_EMIT_HEADER_H_
