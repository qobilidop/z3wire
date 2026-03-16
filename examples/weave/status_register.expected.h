#pragma once

#include <array>
#include <string>
#include <tuple>

#include "status_register.pb.h"
#include "z3wire/bitvec.h"
#include "z3wire/bool.h"
#include "z3wire/int.h"

namespace example {

// =============================================================
// Enum constants
// =============================================================

// Operating mode (width: 2)
struct OpMode {
  static constexpr z3w::UInt<2> kIdle{0};
  static constexpr z3w::UInt<2> kActive{1};
  static constexpr z3w::UInt<2> kSleep{2};
};

// =============================================================
// Concrete types
// =============================================================

// Error information
struct ErrorInfoConcrete {
  z3w::UInt<4> code;
  z3w::SInt<2> severity;
  bool fatal;
  z3w::UInt<1> reserved;

  ErrorInfoProto ToProto() const;
  static ErrorInfoConcrete FromProto(const ErrorInfoProto& proto);
};

// Device status register
struct StatusRegisterConcrete {
  bool ready;
  z3w::UInt<2> mode;
  ErrorInfoConcrete error;
  std::array<z3w::UInt<4>, 4> counters;
  z3w::UInt<5> reserved;

  StatusRegisterProto ToProto() const;
  static StatusRegisterConcrete FromProto(const StatusRegisterProto& proto);
};

// =============================================================
// Symbolic types
// =============================================================

// Error information (symbolic)
// Total width: 8 bits, field pack order: LSB first
struct ErrorInfoSymbolic {
  z3w::Ubv<4> code;      // [3:0]
  z3w::Sbv<2> severity;  // [5:4]
  z3w::Bool fatal;       // [6]
  z3w::Ubv<1> reserved;  // [7]

  static ErrorInfoSymbolic Create(z3::context& ctx, const std::string& prefix);
  z3w::Ubv<8> Pack() const;
  ErrorInfoConcrete ToConcrete(const z3::model& model) const;
  static ErrorInfoSymbolic FromConcrete(z3::context& ctx,
                                        const ErrorInfoConcrete& concrete);
};

// Device status register (symbolic)
// Total width: 32 bits, field pack order: LSB first
struct StatusRegisterSymbolic {
  z3w::Bool ready;                      // [0]
  z3w::Ubv<2> mode;                     // [2:1]
  ErrorInfoSymbolic error;              // [10:3]
  std::array<z3w::Ubv<4>, 4> counters;  // [26:11]
  z3w::Ubv<5> reserved;                 // [31:27]

  static StatusRegisterSymbolic Create(z3::context& ctx,
                                       const std::string& prefix);
  z3w::Ubv<32> Pack() const;
  StatusRegisterConcrete ToConcrete(const z3::model& model) const;
  static StatusRegisterSymbolic FromConcrete(
      z3::context& ctx, const StatusRegisterConcrete& concrete);
};

// =============================================================
// Inline implementations
// =============================================================

inline ErrorInfoProto ErrorInfoConcrete::ToProto() const {
  ErrorInfoProto proto;
  proto.set_code(static_cast<uint32_t>(code.value()));
  proto.set_severity(static_cast<int32_t>(severity.value()));
  proto.set_fatal(fatal);
  return proto;
}

inline ErrorInfoConcrete ErrorInfoConcrete::FromProto(
    const ErrorInfoProto& proto) {
  ErrorInfoConcrete result{};
  result.code = std::get<0>(z3w::UInt<4>::checked(proto.code()));
  result.severity = std::get<0>(z3w::SInt<2>::checked(proto.severity()));
  result.fatal = proto.fatal();
  return result;
}

inline ErrorInfoSymbolic ErrorInfoSymbolic::Create(z3::context& ctx,
                                                   const std::string& prefix) {
  ErrorInfoSymbolic result;
  result.code = z3w::Ubv<4>(ctx, prefix + ".code");
  result.severity = z3w::Sbv<2>(ctx, prefix + ".severity");
  result.fatal = z3w::Bool(ctx, prefix + ".fatal");
  result.reserved = z3w::Ubv<1>(ctx, prefix + ".reserved");
  return result;
}

inline z3w::Ubv<8> ErrorInfoSymbolic::Pack() const {
  return z3w::concat(reserved, z3w::to_ubv1(fatal),
                     z3w::cast<z3w::Ubv<2>>(severity), code);
}

inline ErrorInfoConcrete ErrorInfoSymbolic::ToConcrete(
    const z3::model& model) const {
  ErrorInfoConcrete result{};
  result.code = std::get<0>(z3w::UInt<4>::checked(
      static_cast<uint32_t>(model.eval(code.raw()).get_numeral_int64())));
  result.severity = std::get<0>(z3w::SInt<2>::checked(
      static_cast<int32_t>(model.eval(severity.raw()).get_numeral_int64())));
  result.fatal = model.eval(fatal.raw()).is_true();
  result.reserved = std::get<0>(z3w::UInt<1>::checked(
      static_cast<uint32_t>(model.eval(reserved.raw()).get_numeral_int64())));
  return result;
}

inline ErrorInfoSymbolic ErrorInfoSymbolic::FromConcrete(
    z3::context& ctx, const ErrorInfoConcrete& concrete) {
  ErrorInfoSymbolic result;
  result.code = z3w::to_symbolic(concrete.code, ctx);
  result.severity = z3w::to_symbolic(concrete.severity, ctx);
  result.fatal =
      (concrete.fatal ? z3w::Bool::True(ctx) : z3w::Bool::False(ctx));
  result.reserved = z3w::to_symbolic(concrete.reserved, ctx);
  return result;
}

inline StatusRegisterProto StatusRegisterConcrete::ToProto() const {
  StatusRegisterProto proto;
  proto.set_ready(ready);
  proto.set_mode(static_cast<uint32_t>(mode.value()));
  *proto.mutable_error() = error.ToProto();
  for (size_t i = 0; i < 4; ++i) {
    proto.add_counters(static_cast<uint32_t>(counters[i].value()));
  }
  return proto;
}

inline StatusRegisterConcrete StatusRegisterConcrete::FromProto(
    const StatusRegisterProto& proto) {
  StatusRegisterConcrete result{};
  result.ready = proto.ready();
  result.mode = std::get<0>(z3w::UInt<2>::checked(proto.mode()));
  result.error = ErrorInfoConcrete::FromProto(proto.error());
  for (size_t i = 0; i < 4; ++i) {
    result.counters[i] = std::get<0>(z3w::UInt<4>::checked(proto.counters(i)));
  }
  return result;
}

inline StatusRegisterSymbolic StatusRegisterSymbolic::Create(
    z3::context& ctx, const std::string& prefix) {
  StatusRegisterSymbolic result;
  result.ready = z3w::Bool(ctx, prefix + ".ready");
  result.mode = z3w::Ubv<2>(ctx, prefix + ".mode");
  result.error = ErrorInfoSymbolic::Create(ctx, prefix + ".error");
  for (size_t i = 0; i < 4; ++i) {
    result.counters[i] =
        z3w::Ubv<4>(ctx, prefix + ".counters[" + std::to_string(i) + "]");
  }
  result.reserved = z3w::Ubv<5>(ctx, prefix + ".reserved");
  return result;
}

inline z3w::Ubv<32> StatusRegisterSymbolic::Pack() const {
  return z3w::concat(reserved, counters[3], counters[2], counters[1],
                     counters[0], error.Pack(), mode, z3w::to_ubv1(ready));
}

inline StatusRegisterConcrete StatusRegisterSymbolic::ToConcrete(
    const z3::model& model) const {
  StatusRegisterConcrete result{};
  result.ready = model.eval(ready.raw()).is_true();
  result.mode = std::get<0>(z3w::UInt<2>::checked(
      static_cast<uint32_t>(model.eval(mode.raw()).get_numeral_int64())));
  result.error = error.ToConcrete(model);
  for (size_t i = 0; i < 4; ++i) {
    result.counters[i] =
        std::get<0>(z3w::UInt<4>::checked(static_cast<uint32_t>(
            model.eval(counters[i].raw()).get_numeral_int64())));
  }
  result.reserved = std::get<0>(z3w::UInt<5>::checked(
      static_cast<uint32_t>(model.eval(reserved.raw()).get_numeral_int64())));
  return result;
}

inline StatusRegisterSymbolic StatusRegisterSymbolic::FromConcrete(
    z3::context& ctx, const StatusRegisterConcrete& concrete) {
  StatusRegisterSymbolic result;
  result.ready =
      (concrete.ready ? z3w::Bool::True(ctx) : z3w::Bool::False(ctx));
  result.mode = z3w::to_symbolic(concrete.mode, ctx);
  result.error = ErrorInfoSymbolic::FromConcrete(ctx, concrete.error);
  for (size_t i = 0; i < 4; ++i) {
    result.counters[i] = z3w::to_symbolic(concrete.counters[i], ctx);
  }
  result.reserved = z3w::to_symbolic(concrete.reserved, ctx);
  return result;
}

}  // namespace example
