// Symbolic execution of an exact-match table.
//
// First in a series of symbolic-execution examples. Models an exact-match
// table with concrete entries and a fully-symbolic 48-bit input, and
// enumerates the K+1 execution paths (K hit paths + 1 miss path). For each
// path it prints a path-condition description, a witness input solved from
// that condition, and the corresponding output value.
//
// Run: ./dev.sh bazel run //examples/symbolic_execution:exact_match_table

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

#include <z3++.h>

#include "z3wire/bit_vec.h"
#include "z3wire/sym_bit_vec.h"

namespace {

struct Entry {
  z3w::UInt<24> key;
  z3w::UInt<8> value;
};

struct Table {
  std::array<size_t, 3> offsets;  // byte offsets into input, MSB-first
  std::array<Entry, 4> entries;
};

constexpr Table kTable = {
    .offsets = {0, 2, 4},
    .entries =
        {
            Entry{z3w::UInt<24>::Literal<0xaabbcc>(),
                  z3w::UInt<8>::Literal<0x01>()},
            Entry{z3w::UInt<24>::Literal<0xddeeff>(),
                  z3w::UInt<8>::Literal<0x02>()},
            Entry{z3w::UInt<24>::Literal<0x112233>(),
                  z3w::UInt<8>::Literal<0x03>()},
            Entry{z3w::UInt<24>::Literal<0x445566>(),
                  z3w::UInt<8>::Literal<0x04>()},
        },
};

constexpr uint8_t kMissValue = 0xff;

// Compile-time check that all entry keys are unique. Exact-match tables with
// duplicate keys would be ambiguous; we enforce the invariant rather than
// silently relying on it.
consteval bool all_keys_unique(const Table& t) {
  for (size_t i = 0; i < t.entries.size(); ++i) {
    for (size_t j = i + 1; j < t.entries.size(); ++j) {
      if (t.entries[i].key == t.entries[j].key) {
        return false;
      }
    }
  }
  return true;
}
static_assert(all_keys_unique(kTable), "exact-match table keys must be unique");

// Extract bytes at the given offsets and concatenate them into the match key.
// MSB-first byte numbering: byte 0 is the high byte of the input.
template <size_t N, size_t W>
z3w::SymUInt<N * 8> extract_key(const z3w::SymUInt<W>& input,
                                const std::array<size_t, N>& offsets) {
  z3::expr_vector bytes(input.expr().ctx());
  for (size_t off : offsets) {
    bytes.push_back(z3w::extract<8>(input, W - 8 - 8 * off).expr());
  }
  return z3w::SymUInt<N * 8>::From(z3::concat(bytes));
}

// Print a 48-bit Z3 model value as a 12-digit hex string (no "0x" prefix).
// Z3 returns "#x..." for bit-vectors whose width is a multiple of 4.
std::string input_to_hex(const z3::model& model, const z3::expr& e) {
  std::string s = model.eval(e, true).to_string();
  if (s.size() >= 2 && s.substr(0, 2) == "#x") {
    return s.substr(2);
  }
  return s;
}

}  // namespace

int main() {
  z3::context ctx;
  z3w::SymUInt<48> input(ctx, "input");
  auto key = extract_key(input, kTable.offsets);

  std::cout << "Exact-match table: " << kTable.entries.size()
            << " entries, key bytes at offsets {";
  for (size_t i = 0; i < kTable.offsets.size(); ++i) {
    std::cout << (i == 0 ? "" : ", ") << kTable.offsets[i];
  }
  std::cout << "}\n\n";

  // Hit paths: one per entry. Under the unique-key invariant, each
  // s.check() is always sat.
  for (size_t i = 0; i < kTable.entries.size(); ++i) {
    const auto& e = kTable.entries[i];
    z3::solver s(ctx);
    s.add((key == e.key).expr());
    s.check();
    auto m = s.get_model();
    std::cout << "Path " << i << ": hit (entry " << i << ", key=0x" << std::hex
              << std::setw(6) << std::setfill('0') << e.key.value()
              << ", value=0x" << std::setw(2)
              << static_cast<unsigned>(e.value.value()) << ")\n";
    std::cout << "  Path condition: key(input) == 0x" << std::setw(6)
              << e.key.value() << "\n";
    std::cout << "  Witness input:  0x" << input_to_hex(m, input.expr())
              << "\n";
    std::cout << "  Output:         0x" << std::setw(2)
              << static_cast<unsigned>(e.value.value()) << "\n\n";
    std::cout << std::dec << std::setfill(' ');
  }

  // Miss path: key not equal to any entry's key. Always sat under our
  // dimensions (4 entries cannot cover the 24-bit key space).
  {
    z3::solver s(ctx);
    for (const auto& e : kTable.entries) {
      s.add((key != e.key).expr());
    }
    s.check();
    auto m = s.get_model();
    std::cout << "Path " << kTable.entries.size() << ": miss\n";
    std::cout << "  Path condition: key(input) not in {";
    for (size_t i = 0; i < kTable.entries.size(); ++i) {
      std::cout << (i == 0 ? "" : ", ") << "0x" << std::hex << std::setw(6)
                << std::setfill('0') << kTable.entries[i].key.value();
    }
    std::cout << "}\n";
    std::cout << "  Witness input:  0x" << input_to_hex(m, input.expr())
              << "\n";
    std::cout << "  Output:         0x" << std::setw(2)
              << static_cast<unsigned>(kMissValue) << " (miss)\n";
    std::cout << std::dec << std::setfill(' ');
  }

  return 0;
}
