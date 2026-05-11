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

  z3w::SymUInt<32> value() const {
    return z3w::extract<31 + BaseOffset, 0 + BaseOffset>(*buf_);
  }
  void set_value(const z3w::SymUInt<32>& v) {
    *buf_ = z3w::replace<0 + BaseOffset>(*buf_, v);
  }

  z3w::SymUInt<8> octet0() const {
    return z3w::extract<31 + BaseOffset, 24 + BaseOffset>(*buf_);
  }
  void set_octet0(const z3w::SymUInt<8>& v) {
    *buf_ = z3w::replace<24 + BaseOffset>(*buf_, v);
  }

  z3w::SymUInt<8> octet1() const {
    return z3w::extract<23 + BaseOffset, 16 + BaseOffset>(*buf_);
  }
  void set_octet1(const z3w::SymUInt<8>& v) {
    *buf_ = z3w::replace<16 + BaseOffset>(*buf_, v);
  }

  z3w::SymUInt<8> octet2() const {
    return z3w::extract<15 + BaseOffset, 8 + BaseOffset>(*buf_);
  }
  void set_octet2(const z3w::SymUInt<8>& v) {
    *buf_ = z3w::replace<8 + BaseOffset>(*buf_, v);
  }

  z3w::SymUInt<8> octet3() const {
    return z3w::extract<7 + BaseOffset, 0 + BaseOffset>(*buf_);
  }
  void set_octet3(const z3w::SymUInt<8>& v) {
    *buf_ = z3w::replace<0 + BaseOffset>(*buf_, v);
  }

 private:
  z3w::SymUInt<BufW>* buf_;
};

int main() { return 0; }
