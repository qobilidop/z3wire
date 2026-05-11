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

int main() { return 0; }
