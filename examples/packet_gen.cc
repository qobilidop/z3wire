// Generate valid Ethernet + IPv4/IPv6 packets using symbolic constraints.
//
// Models a 54-byte symbolic packet buffer and solves twice: once constrained to
// be a valid IPv4 packet (dst in 10.0.0.0/8), once constrained to be a valid
// IPv6 packet. Prints each solution as a hex string that can be pasted into
// https://hpd.gasmi.net/ for verification.
//
// Run: ./dev.sh bazel run //examples:packet_gen

#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>

#include <z3++.h>

#include "z3wire/sym_bit_vec.h"

// Print a Z3 bit-vector model value as a hex string (no prefix).
std::string to_hex(const z3::model& model, const z3::expr& e) {
  // Z3 returns "#x..." for bit-vectors whose width is a multiple of 4.
  std::string s = model.eval(e, true).to_string();
  if (s.substr(0, 2) == "#x") {
    return s.substr(2);
  }
  // Fallback for binary format "#b...": convert to hex.
  std::string bits = s.substr(2);
  // Pad to multiple of 4.
  while (bits.size() % 4 != 0) {
    bits.insert(bits.begin(), '0');
  }
  std::ostringstream hex;
  for (size_t i = 0; i < bits.size(); i += 4) {
    int nibble = 0;
    for (int j = 0; j < 4; ++j) {
      nibble = (nibble << 1) | (bits[i + j] - '0');
    }
    hex << std::hex << nibble;
  }
  return hex.str();
}

int main() {
  z3::context ctx;

  // 54-byte (432-bit) symbolic packet buffer.
  z3w::SymUInt<432> packet(ctx, "packet");

  // --- Ethernet header (bytes 0-13, bits 431:320) ---
  // auto dst_mac = z3w::extract<431, 384>(packet);   // 48 bits
  // auto src_mac = z3w::extract<383, 336>(packet);   // 48 bits
  auto ethertype = z3w::extract<335, 320>(packet);  // 16 bits

  // --- IP header (bytes 14-53, bits 319:0) ---
  // First nibble is IP version in both IPv4 and IPv6.
  auto version = z3w::extract<319, 316>(packet);  // 4 bits

  // -- IPv4 fields (when ethertype = 0x0800) --
  auto ipv4_ihl = z3w::extract<315, 312>(packet);              // 4 bits
  auto ipv4_dscp_ecn = z3w::extract<311, 304>(packet);         // 8 bits
  auto ipv4_total_len = z3w::extract<303, 288>(packet);        // 16 bits
  auto ipv4_id = z3w::extract<287, 272>(packet);               // 16 bits
  auto ipv4_flags_frag = z3w::extract<271, 256>(packet);       // 16 bits
  auto ipv4_ttl = z3w::extract<255, 248>(packet);              // 8 bits
  auto ipv4_protocol = z3w::extract<247, 240>(packet);         // 8 bits
  auto ipv4_checksum = z3w::extract<239, 224>(packet);         // 16 bits
  auto ipv4_src_addr = z3w::extract<223, 192>(packet);         // 32 bits
  auto ipv4_dst_addr = z3w::extract<191, 160>(packet);         // 32 bits
  auto ipv4_dst_first_octet = z3w::extract<191, 184>(packet);  // 8 bits

  // -- IPv6 fields (when ethertype = 0x86DD) --
  auto ipv6_traffic_flow = z3w::extract<315, 288>(packet);  // 28 bits
  auto ipv6_payload_len = z3w::extract<287, 272>(packet);   // 16 bits
  auto ipv6_next_header = z3w::extract<271, 264>(packet);   // 8 bits
  auto ipv6_hop_limit = z3w::extract<263, 256>(packet);     // 8 bits
  // auto ipv6_src_addr = z3w::extract<255, 128>(packet);     // 128 bits
  // auto ipv6_dst_addr = z3w::extract<127, 0>(packet);       // 128 bits

  // --- Solve for a valid IPv4 packet ---
  {
    z3::solver solver(ctx);

    // Ethernet: EtherType = 0x0800 (IPv4).
    solver.add((ethertype == z3w::SymUInt<16>::Literal<0x0800>(ctx)).expr());

    // IPv4 header constraints.
    solver.add((version == z3w::SymUInt<4>::Literal<4>(ctx)).expr());
    solver.add((ipv4_ihl == z3w::SymUInt<4>::Literal<5>(ctx)).expr());
    solver.add((ipv4_dscp_ecn == z3w::SymUInt<8>::Literal<0>(ctx)).expr());
    // Total length = 40 (20-byte header + 20 bytes remaining).
    solver.add((ipv4_total_len == z3w::SymUInt<16>::Literal<40>(ctx)).expr());
    solver.add((ipv4_id == z3w::SymUInt<16>::Literal<0>(ctx)).expr());
    solver.add((ipv4_flags_frag == z3w::SymUInt<16>::Literal<0>(ctx)).expr());
    // Protocol = 253 (experimental/testing, RFC 3692).
    solver.add((ipv4_protocol == z3w::SymUInt<8>::Literal<253>(ctx)).expr());
    solver.add((ipv4_checksum == z3w::SymUInt<16>::Literal<0>(ctx)).expr());

    // Interesting constraint: dst in 10.0.0.0/8, TTL > 0.
    solver.add(
        (ipv4_dst_first_octet == z3w::SymUInt<8>::Literal<10>(ctx)).expr());
    solver.add((ipv4_ttl > z3w::SymUInt<8>::Literal<0>(ctx)).expr());

    if (solver.check() == z3::sat) {
      auto model = solver.get_model();
      std::cout << "IPv4 packet (dst in 10.0.0.0/8):\n";
      std::cout << to_hex(model, packet.expr()) << "\n\n";
    } else {
      std::cout << "No IPv4 solution (unexpected).\n";
    }
  }

  // --- Solve for a valid IPv6 packet ---
  {
    z3::solver solver(ctx);

    // Ethernet: EtherType = 0x86DD (IPv6).
    solver.add((ethertype == z3w::SymUInt<16>::Literal<0x86DD>(ctx)).expr());

    // IPv6 header constraints.
    solver.add((version == z3w::SymUInt<4>::Literal<6>(ctx)).expr());
    solver.add((ipv6_traffic_flow == z3w::SymUInt<28>::Literal<0>(ctx)).expr());
    // Payload length = 0 (header only).
    solver.add((ipv6_payload_len == z3w::SymUInt<16>::Literal<0>(ctx)).expr());
    // Next header = 59 (No Next Header).
    solver.add((ipv6_next_header == z3w::SymUInt<8>::Literal<59>(ctx)).expr());
    // Hop limit > 0.
    solver.add((ipv6_hop_limit > z3w::SymUInt<8>::Literal<0>(ctx)).expr());

    if (solver.check() == z3::sat) {
      auto model = solver.get_model();
      std::cout << "IPv6 packet:\n";
      std::cout << to_hex(model, packet.expr()) << "\n\n";
    } else {
      std::cout << "No IPv6 solution (unexpected).\n";
    }
  }

  std::cout << "Paste the hex strings into https://hpd.gasmi.net/ to verify.\n";

  return 0;
}
