# Weave

## Purpose

Weave is a codegen tool that generates C++ structs and protobuf messages from a
single source-of-truth (SoT) description of hierarchical bit field data
structures. It eliminates the manual effort of keeping symbolic structs,
concrete structs, and serialization formats in sync.

## Motivation

When modeling hardware registers, wire protocols, or arbitrary bit-packed
layouts with Z3Wire, users currently write boilerplate by hand:

- A symbolic struct with `z3w::SymBool`, `z3w::SymUInt<W>`, `z3w::SymSInt<W>`
    fields.
- A concrete struct with `z3w::Bool`, `z3w::UInt<W>`, `z3w::SInt<W>` fields.
- Conversion methods between them.
- `extract` / `concat` logic to pack and unpack flat bit-vectors.
- A protobuf message for serialization.

All of these must agree on field names, widths, and ordering. Any mismatch is a
silent bug. Weave generates all of them from a single description.

## Scope

Weave covers **fixed-layout bit structures** — the kind found in hardware
registers, protocol headers, and control/status words. It does not cover:

- Variable-length fields or dynamic arrays.
- Address maps or register files (e.g. SystemRDL `addrmap` / `regfile`).
- Access properties (`sw`, `hw`, `reset`) — these describe register behavior,
    not bit layout.

## Source-of-truth format: wire spec

The source-of-truth is defined using Protocol Buffers:

- **`wire_spec.proto`** defines the schema (what a wire spec description looks
    like).
- **`.txtpb` files** are human-readable instances conforming to that schema.

This approach avoids writing a custom parser - protobuf handles parsing and
validation. Users get a familiar, well-tooled format.

### Schema (`wire_spec.proto`)

```protobuf
syntax = "proto3";
package z3wire_weave;

enum FieldPackOrder {
  FIELD_PACK_ORDER_UNSPECIFIED = 0;
  FIELD_PACK_ORDER_LSB_FIRST = 1;
  FIELD_PACK_ORDER_MSB_FIRST = 2;
}

enum Signedness {
  SIGNEDNESS_UNSIGNED = 0;
  SIGNEDNESS_SIGNED = 1;
}

message WireSpec {
  string file_prefix = 1;
  string namespace = 2;
  FieldPackOrder field_pack_order = 3;
  repeated EnumDef enums = 4;
  repeated Struct structs = 5;
}

message EnumDef {
  string name = 1;
  string desc = 2;
  uint32 width = 3;
  repeated EnumValue values = 4;
}

message EnumValue {
  string name = 1;
  string desc = 2;
  uint64 value = 3;
}

message Struct {
  string name = 1;
  string desc = 2;
  optional uint32 width = 3;
  FieldPackOrder field_pack_order = 4;
  repeated Field fields = 5;
}

message Field {
  string name = 1;
  string desc = 2;
  FieldType type = 3;
  uint32 count = 4;
  bool reserved = 5;
}

message FieldType {
  oneof kind {
    BoolType bool = 1;
    BitVecType bitvec = 2;
    string enum_ref = 3;
    string struct_ref = 4;
  }
}

message BoolType {}

message BitVecType {
  uint32 width = 1;
  Signedness signedness = 2;
}
```

### Key schema concepts

- **WireSpec**: Top-level container. Sets the `file_prefix` (output filename
    prefix), C++ `namespace`, and a default `field_pack_order` inherited by all
    structs.
- **Struct**: A named collection of fields occupying a contiguous bit range. Can
    override the module-level `field_pack_order`. If `width` is set, the tool
    validates that fields sum to exactly that width.
- **Field**: A named bit range within a struct. Types include `bool` (1 bit),
    `bitvec` (unsigned or signed, arbitrary width), `enum_ref` (reference to an
    `EnumDef`), and `struct_ref` (inline nested struct). Setting `count >= 1`
    makes it a fixed-size array (`std::array<T, N>` in C++); `count = 0`
    (default) means scalar. Zero-length arrays are not supported. Setting
    `reserved = true` marks a field as padding — it is included in the C++
    structs (for correct bit layout) but omitted from the generated proto.
- **EnumDef**: Named constants with explicit bit-pattern values and a declared
    width.
- **FieldPackOrder**: Controls the direction fields are packed into the bit
    range. `FIELD_PACK_ORDER_LSB_FIRST` packs starting from bit 0 (hardware
    registers). `FIELD_PACK_ORDER_MSB_FIRST` packs starting from the most
    significant bit (network protocols). Resolved by inheritance: struct
    overrides module; if both are unspecified, codegen errors.

### Example input

```textproto
# status_register.wire_spec.txtpb

file_prefix: "status_register"
namespace: "example"
field_pack_order: FIELD_PACK_ORDER_LSB_FIRST

enums {
  name: "OpMode"
  desc: "Operating mode"
  width: 2
  values { name: "kIdle"   value: 0 }
  values { name: "kActive" value: 1 }
  values { name: "kSleep"  value: 2 }
}

structs {
  name: "ErrorInfo"
  desc: "Error information"
  width: 8
  fields { name: "code"     type { bitvec { width: 4 } } }
  fields { name: "severity" type { bitvec { width: 2 signedness: SIGNEDNESS_SIGNED } } }
  fields { name: "fatal"    type { bool {} } }
  fields { name: "reserved" type { bitvec { width: 1 } } reserved: true }
}

structs {
  name: "StatusRegister"
  desc: "Device status register"
  width: 32
  fields { name: "ready"    type { bool {} } }
  fields { name: "mode"     type { enum_ref: "OpMode" } }
  fields { name: "error"    type { struct_ref: "ErrorInfo" } }
  fields { name: "counters" type { bitvec { width: 4 } } count: 4 }
  fields { name: "reserved" type { bitvec { width: 5 } } reserved: true }
}
```

This exercises all supported features: bool, unsigned bitvec, signed bitvec,
enum reference, nested struct, fixed-size array, width validation, and
descriptions.

## Generated outputs

Weave produces two files per input: one C++ header and one proto file.

### C++ header (`status_register.h`)

The header is organized into four labeled sections: enum constants, concrete
types, symbolic types, and inline implementations. Declarations are at the top
for quick scanning; method bodies follow at the bottom.

```cpp
#pragma once

#include <array>
#include <string>
#include <tuple>

#include "z3wire/bool.h"
#include "z3wire/sym_bool.h"
#include "z3wire/sym_bit_vec.h"
#include "z3wire/bit_vec.h"
#include "status_register.pb.h"

namespace example {

// =============================================================
// Enum constants
// =============================================================

// Operating mode (width: 2)
struct OpMode {
  static constexpr auto kIdle = z3w::UInt<2>::Literal<0>();
  static constexpr auto kActive = z3w::UInt<2>::Literal<1>();
  static constexpr auto kSleep = z3w::UInt<2>::Literal<2>();
};

// =============================================================
// Concrete types
// =============================================================

// Error information
struct ErrorInfoConcrete {
  z3w::UInt<4> code;
  z3w::SInt<2> severity;
  z3w::Bool fatal;
  z3w::UInt<1> reserved;

  ErrorInfoProto ToProto() const;
  static ErrorInfoConcrete FromProto(const ErrorInfoProto& proto);
};

// Device status register
struct StatusRegisterConcrete {
  z3w::Bool ready;
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
  z3w::SymUInt<4> code;  // [3:0]
  z3w::SymSInt<2> severity;  // [5:4]
  z3w::SymBool fatal;  // [6]
  z3w::SymUInt<1> reserved;  // [7]

  static ErrorInfoSymbolic Create(z3::context& ctx,
      const std::string& prefix);
  z3w::SymUInt<8> Pack() const;
  ErrorInfoConcrete ToConcrete(const z3::model& model) const;
  static ErrorInfoSymbolic FromConcrete(z3::context& ctx,
      const ErrorInfoConcrete& concrete);
};

// Device status register (symbolic)
// Total width: 32 bits, field pack order: LSB first
struct StatusRegisterSymbolic {
  z3w::SymBool ready;  // [0]
  z3w::SymUInt<2> mode;  // [2:1]
  ErrorInfoSymbolic error;  // [10:3]
  std::array<z3w::SymUInt<4>, 4> counters;  // [26:11]
  z3w::SymUInt<5> reserved;  // [31:27]

  static StatusRegisterSymbolic Create(z3::context& ctx,
      const std::string& prefix);
  z3w::SymUInt<32> Pack() const;
  StatusRegisterConcrete ToConcrete(const z3::model& model) const;
  static StatusRegisterSymbolic FromConcrete(z3::context& ctx,
      const StatusRegisterConcrete& concrete);
};

// =============================================================
// Inline implementations
// =============================================================

// ... (method bodies for ToProto, FromProto, Create, Pack,
//      ToConcrete, FromConcrete — see generated output)

}  // namespace example
```

#### Naming convention

Generated code follows the
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html):
CamelCase for all function names. This differs from the hand-written Z3Wire
library which uses `snake_case` for functions — the distinction is intentional,
as weave-generated code is a separate codebase with its own conventions.

#### Enum constants

Enums are generated as structs with `static constexpr` members using Z3Wire
concrete types. Names are emitted as-is from the wire spec - no transformation.
Users write target-language names directly (e.g., `kIdle` not `IDLE`).

#### Concrete structs

POD data holders with no Z3 dependency. They hold field values using Z3Wire
concrete types (`Bool`, `UInt<W>`, `SInt<W>`) and provide proto conversion
methods (`ToProto`, `FromProto`). Nested structs become concrete struct members.
Arrays become `std::array`.

#### Symbolic structs

Mirror the concrete layout using Z3Wire symbolic types (`SymBool`, `SymUInt<W>`,
`SymSInt<W>`). Generated methods:

- **`Create(ctx, prefix)`**: Creates fresh Z3 symbolic variables. Each variable
    is named `prefix.field_name`. Nested structs chain the prefix automatically
    (e.g., `"sr.error.code"`).
- **`Pack()`**: Packs all fields into a flat bit-vector using `concat`,
    following the declared `field_pack_order`. For the structural constraint
    `bv == reg.Pack()`, users call `Pack()` directly.
- **`ToConcrete(model)`**: Evaluates symbolic fields against a Z3 model to
    produce a concrete struct.
- **`FromConcrete(ctx, concrete)`**: Constructs a symbolic struct from concrete
    values (each field becomes a Z3 constant expression).

### Wire proto (`status_register.proto`)

```protobuf
syntax = "proto3";
package example;

// Error information
message ErrorInfoProto {
  uint32 code = 1;
  sint32 severity = 2;
  bool fatal = 3;
}

// Device status register
message StatusRegisterProto {
  bool ready = 1;
  uint32 mode = 2;
  ErrorInfoProto error = 3;
  repeated uint32 counters = 4;
}
```

Fields marked `reserved: true` are omitted from the proto — there is no value in
serializing padding bits. Nested structs become nested proto messages. Arrays
become `repeated` fields. Signed bitvec fields use proto `sint32`/`sint64`.
Proto integer type selection: `uint32`/`sint32` for widths 1–32,
`uint64`/`sint64` for widths 33–64.

## Conversion chain

```
Symbolic  ↔  Concrete  ↔  Proto
```

Each layer only knows about the layer next to it:

- **Symbolic → Concrete**: `ToConcrete(model)` evaluates Z3 expressions.
- **Concrete → Symbolic**: `FromConcrete(ctx, concrete)` creates Z3 constants.
- **Concrete → Proto**: `ToProto()` serializes field values.
- **Proto → Concrete**: `FromProto(proto)` deserializes field values.

To go from Symbolic to Proto, chain: `sym.ToConcrete(model).ToProto()`.

## Tool architecture

### Directory layout

```
z3wire_weave/
  wire_spec.proto        # Wire spec schema definition
  weave_main.cc          # CLI entry point
  resolver.h             # Validates refs, computes widths and bit offsets
  resolver.cc
  emit_header.h          # Generates the .h file
  emit_header.cc
  emit_proto.h           # Generates the .proto file
  emit_proto.cc
  BUILD.bazel
examples/weave/
  status_register.wire_spec.txtpb  # Example wire spec instance
  BUILD.bazel
```

### CLI

```bash
weave --input status_register.wire_spec.txtpb --output_dir gen/
```

Produces `gen/status_register.h` and `gen/status_register.proto` (filenames
derived from the `file_prefix` field in the WireSpec, not the input filename).

### Implementation

- **Language**: C++. Single language toolchain, simpler packaging.
- **Parsing**: Protobuf's C++ TextFormat API parses and validates `.txtpb` files
    against `wire_spec.proto`. No custom parser.
- **Resolution**: `resolver.cc` validates all references, detects circular
    struct dependencies, computes per-field bit offsets, and validates total
    widths.
- **Emission**: `emit_header.cc` and `emit_proto.cc` generate output using
    Abseil string utilities (`absl::StrCat`, `absl::StrAppend`,
    `absl::StrFormat`). No template engine — string building uses Abseil.
- **Deployment**: Standalone binary for now. Bazel rule integration is future
    work.

### Validation

Weave validates:

- All `enum_ref` and `struct_ref` names resolve to defined types.
- Enum values fit within the declared width.
- Enum values are unique within an `EnumDef`.
- No circular struct references.
- `field_pack_order` is resolved (at module or struct level).
- If `Struct.width` is set, fields sum to exactly that width.
- Field names are valid C++ identifiers and do not collide with generated method
    names (`Create`, `Pack`, `ToConcrete`, `FromConcrete`, `ToProto`,
    `FromProto`).

## Design decisions

| Decision            | Choice                                  | Rationale                                                         |
| ------------------- | --------------------------------------- | ----------------------------------------------------------------- |
| SoT format          | Proto `.txtpb`                          | No custom parser needed                                           |
| Codegen language    | C++                                     | Single language toolchain, simpler packaging                      |
| Field layout        | Ordered, not positioned                 | Tool computes offsets from order + widths                         |
| Field pack order    | Per-module default, per-struct override | Inheritance with explicit override                                |
| Enums               | Struct with `constexpr` constants       | Avoids type mismatch with bit-vector fields                       |
| Name transformation | None                                    | Users write target-language names directly                        |
| Arrays              | Fixed-size only                         | Hardware has no dynamic allocation                                |
| Nesting             | Inline embedding (contiguous bits)      | Natural for hardware/protocol layouts                             |
| Nested prefixes     | Auto-derived                            | User provides top-level prefix only                               |
| Conversions         | Symbolic ↔ Concrete ↔ Proto             | Clean chain, each layer knows only its neighbor                   |
| Concrete struct     | POD data holder                         | No Z3 dependency, minimal methods                                 |
| Reserved fields     | Explicit `reserved` flag on Field       | Omitted from proto; included in C++ for correct layout            |
| Generated files     | One `.h` + one `.proto` per input       | Simplicity; split later if needed                                 |
| Templating          | Abseil string utilities                 | No extra dependency (Abseil is transitively required by protobuf) |
| Width validation    | Optional `width` field on Struct        | Catches field sum mismatches                                      |
| Method naming       | Google C++ Style Guide (CamelCase)      | Generated code is a separate codebase                             |

## Future work

- Bazel rule / protoc plugin integration.
- Optional name transformation (e.g., UPPER_SNAKE to kCamelCase).
- Variable-length arrays.
- Ergonomic symbolic enum comparisons.
- Multi-file imports (referencing types across `.txtpb` files).
- Explicit bit position overrides (placing a field at a specific bit range).
- Default / reset values for fields.
- `Unpack()` — construct a symbolic struct from a flat bit-vector using
    `extract`.
