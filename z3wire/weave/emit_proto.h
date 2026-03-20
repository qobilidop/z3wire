#ifndef Z3WIRE_WEAVE_EMIT_PROTO_H_
#define Z3WIRE_WEAVE_EMIT_PROTO_H_

#include <string>

#include "z3wire/weave/resolver.h"

namespace z3wire::weave {

std::string EmitProto(const ResolvedModule& module);

}  // namespace z3wire::weave

#endif  // Z3WIRE_WEAVE_EMIT_PROTO_H_
