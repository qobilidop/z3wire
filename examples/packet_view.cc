// Demonstrate a structured view layered over a raw symbolic bit-vector
// buffer. The same 54-byte Ethernet + IPv4 packet that packet_gen.cc threads
// as flat extract/replace calls is exposed here as a nested view hierarchy:
//
//   buffer (SymUInt<432>)
//     -> hdr (HdrView)
//          -> hdr.eth, hdr.ipv4   (header views)
//               -> hdr.ipv4.src_addr, hdr.ipv4.dst_addr   (sub-views)
//
// Each view is templated on (BufW, BaseOffset). Field positions inside a
// view are LSB-relative; the view adds BaseOffset to translate into absolute
// bit positions in the buffer. Writes at any level rewrite the same
// SymUInt<W>* via z3w::replace, so mutations propagate to the root buffer
// regardless of nesting depth.
//
// Run: ./dev.sh bazel run //examples:packet_view

#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>

#include <z3++.h>

#include "z3wire/bit_vec.h"
#include "z3wire/sym_bit_vec.h"

// --- View classes ---

// 32-bit IPv4 address, decomposed into four octets.
//
// Local layout (LSB-relative, 32 bits):
//   [31:24] octet0   (first in dotted notation, e.g. 192 in 192.168.0.1)
//   [23:16] octet1
//   [15:8]  octet2
//   [7:0]   octet3
template <int BufW, int BaseOffset>
class Ipv4AddrView {
 public:
  explicit Ipv4AddrView(z3w::SymUInt<BufW>* buf) : buf_(buf) {}

  [[nodiscard]] z3w::SymUInt<32> value() const {
    return z3w::extract<31 + BaseOffset, 0 + BaseOffset>(*buf_);
  }
  void set_value(const z3w::SymUInt<32>& v) {
    *buf_ = z3w::replace<0 + BaseOffset>(*buf_, v);
  }

  [[nodiscard]] z3w::SymUInt<8> octet0() const {
    return z3w::extract<31 + BaseOffset, 24 + BaseOffset>(*buf_);
  }
  void set_octet0(const z3w::SymUInt<8>& v) {
    *buf_ = z3w::replace<24 + BaseOffset>(*buf_, v);
  }

  [[nodiscard]] z3w::SymUInt<8> octet1() const {
    return z3w::extract<23 + BaseOffset, 16 + BaseOffset>(*buf_);
  }
  void set_octet1(const z3w::SymUInt<8>& v) {
    *buf_ = z3w::replace<16 + BaseOffset>(*buf_, v);
  }

  [[nodiscard]] z3w::SymUInt<8> octet2() const {
    return z3w::extract<15 + BaseOffset, 8 + BaseOffset>(*buf_);
  }
  void set_octet2(const z3w::SymUInt<8>& v) {
    *buf_ = z3w::replace<8 + BaseOffset>(*buf_, v);
  }

  [[nodiscard]] z3w::SymUInt<8> octet3() const {
    return z3w::extract<7 + BaseOffset, 0 + BaseOffset>(*buf_);
  }
  void set_octet3(const z3w::SymUInt<8>& v) {
    *buf_ = z3w::replace<0 + BaseOffset>(*buf_, v);
  }

 private:
  z3w::SymUInt<BufW>* buf_;
};

// IPv4 fixed (option-less) header.
//
// Local layout (LSB-relative, 160 bits / 20 bytes):
//   [159:156] version    [155:152] ihl       [151:144] dscp_ecn
//   [143:128] total_len  [127:112] id        [111:96]  flags_frag
//   [95:88]   ttl        [87:80]   protocol  [79:64]   checksum
//   [63:32]   src_addr   [31:0]    dst_addr
//
// Only a representative subset of the 14 IPv4 fields is modeled (version,
// ihl, dscp_ecn, total_len, src_addr, dst_addr). The other fields follow
// the same pattern and would add no new design content.
template <int BufW, int BaseOffset>
class Ipv4View {
 public:
  explicit Ipv4View(z3w::SymUInt<BufW>* buf)
      : src_addr(buf), dst_addr(buf), buf_(buf) {}

  [[nodiscard]] z3w::SymUInt<4> version() const {
    return z3w::extract<159 + BaseOffset, 156 + BaseOffset>(*buf_);
  }
  void set_version(const z3w::SymUInt<4>& v) {
    *buf_ = z3w::replace<156 + BaseOffset>(*buf_, v);
  }

  [[nodiscard]] z3w::SymUInt<4> ihl() const {
    return z3w::extract<155 + BaseOffset, 152 + BaseOffset>(*buf_);
  }
  void set_ihl(const z3w::SymUInt<4>& v) {
    *buf_ = z3w::replace<152 + BaseOffset>(*buf_, v);
  }

  [[nodiscard]] z3w::SymUInt<8> dscp_ecn() const {
    return z3w::extract<151 + BaseOffset, 144 + BaseOffset>(*buf_);
  }
  void set_dscp_ecn(const z3w::SymUInt<8>& v) {
    *buf_ = z3w::replace<144 + BaseOffset>(*buf_, v);
  }

  [[nodiscard]] z3w::SymUInt<16> total_len() const {
    return z3w::extract<143 + BaseOffset, 128 + BaseOffset>(*buf_);
  }
  void set_total_len(const z3w::SymUInt<16>& v) {
    *buf_ = z3w::replace<128 + BaseOffset>(*buf_, v);
  }

  // Decomposed sub-views, pinned at the appropriate IPv4-local offsets.
  Ipv4AddrView<BufW, BaseOffset + 32> src_addr;  // IPv4-local [63:32]
  Ipv4AddrView<BufW, BaseOffset + 0> dst_addr;   // IPv4-local [31:0]

 private:
  z3w::SymUInt<BufW>* buf_;
};

// Ethernet (DIX) header.
//
// Local layout (LSB-relative, 112 bits / 14 bytes):
//   [111:64] dst_mac (48)
//   [63:16]  src_mac (48)
//   [15:0]   ethertype (16)
template <int BufW, int BaseOffset>
class EthView {
 public:
  explicit EthView(z3w::SymUInt<BufW>* buf) : buf_(buf) {}

  [[nodiscard]] z3w::SymUInt<48> dst_mac() const {
    return z3w::extract<111 + BaseOffset, 64 + BaseOffset>(*buf_);
  }
  void set_dst_mac(const z3w::SymUInt<48>& v) {
    *buf_ = z3w::replace<64 + BaseOffset>(*buf_, v);
  }

  [[nodiscard]] z3w::SymUInt<48> src_mac() const {
    return z3w::extract<63 + BaseOffset, 16 + BaseOffset>(*buf_);
  }
  void set_src_mac(const z3w::SymUInt<48>& v) {
    *buf_ = z3w::replace<16 + BaseOffset>(*buf_, v);
  }

  [[nodiscard]] z3w::SymUInt<16> ethertype() const {
    return z3w::extract<15 + BaseOffset, 0 + BaseOffset>(*buf_);
  }
  void set_ethertype(const z3w::SymUInt<16>& v) {
    *buf_ = z3w::replace<0 + BaseOffset>(*buf_, v);
  }

 private:
  z3w::SymUInt<BufW>* buf_;
};

// Top-level composite for a 54-byte Ethernet + IPv4 packet.
//
// Buffer layout (LSB-relative bit positions):
//   [431:320] Ethernet header (14 bytes)
//   [319:160] IPv4 header     (20 bytes)
//   [159:0]   IPv4 payload    (20 bytes, not modeled)
template <int BufW>
class HdrView {
 public:
  static_assert(BufW >= 432, "HdrView requires BufW >= 432 bits.");

  explicit HdrView(z3w::SymUInt<BufW>* buf) : eth(buf), ipv4(buf) {}

  EthView<BufW, 320> eth;
  Ipv4View<BufW, 160> ipv4;
};

// --- Model printer ---

// Print a Z3 bit-vector model value as a hex string (no prefix). Same shape
// as the helper in packet_gen.cc; duplicated rather than shared to keep the
// example self-contained.
std::string to_hex(const z3::model& model, const z3::expr& e) {
  std::string s = model.eval(e, true).to_string();
  if (s.substr(0, 2) == "#x") {
    return s.substr(2);
  }
  std::string bits = s.substr(2);
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

  // 54-byte symbolic packet buffer.
  z3w::SymUInt<432> buffer(ctx, "packet");

  // Structured view over the same buffer.
  HdrView<432> hdr(&buffer);

  z3::solver solver(ctx);

  // Constrain to a valid IPv4 packet with source address in 10.0.0.0/8.
  // The constraints mix Level-1 leaf fields (e.g. hdr.ipv4.ihl()) with a
  // Level-2 sub-view field (hdr.ipv4.src_addr.octet0()) in one set,
  // demonstrating that the hierarchy doesn't leak into the call-site API.
  solver.add((hdr.eth.ethertype() == z3w::UInt<16>::Literal<0x0800>()).expr());
  solver.add((hdr.ipv4.version() == z3w::UInt<4>::Literal<4>()).expr());
  solver.add((hdr.ipv4.ihl() == z3w::UInt<4>::Literal<5>()).expr());
  solver.add((hdr.ipv4.total_len() == z3w::UInt<16>::Literal<40>()).expr());
  solver.add(
      (hdr.ipv4.src_addr.octet0() == z3w::UInt<8>::Literal<10>()).expr());

  if (solver.check() != z3::sat) {
    std::cerr << "Solve failed\n";
    return 1;
  }
  z3::model m = solver.get_model();
  std::cout << "packet = " << to_hex(m, buffer.expr()) << "\n";
  return 0;
}
