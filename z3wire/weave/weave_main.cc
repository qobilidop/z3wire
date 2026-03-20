// Weave: generate C++ and proto files from Z3Wire RDL descriptions.

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "google/protobuf/text_format.h"
#include "z3wire/weave/emit_header.h"
#include "z3wire/weave/emit_proto.h"
#include "z3wire/weave/rdl.pb.h"
#include "z3wire/weave/resolver.h"

namespace {

struct Args {
  std::string input;
  std::string output_dir;
};

Args ParseArgs(int argc, char* argv[]) {
  Args args;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--input" && i + 1 < argc) {
      args.input = argv[++i];
    } else if (arg == "--output_dir" && i + 1 < argc) {
      args.output_dir = argv[++i];
    }
  }
  if (args.input.empty() || args.output_dir.empty()) {
    std::cerr << "Usage: weave --input <path> --output_dir <path>\n";
    std::exit(1);
  }
  return args;
}

}  // namespace

int main(int argc, char* argv[]) {
  Args args = ParseArgs(argc, argv);

  // Read input file.
  std::ifstream input_file(args.input);
  if (!input_file) {
    std::cerr << "Error: cannot open " << args.input << "\n";
    return 1;
  }
  std::ostringstream ss;
  ss << input_file.rdbuf();
  std::string text = ss.str();

  // Parse textproto.
  z3wire_rdl::Module module;
  if (!google::protobuf::TextFormat::ParseFromString(text, &module)) {
    std::cerr << "Error: failed to parse " << args.input << "\n";
    return 1;
  }

  // Resolve.
  z3wire::weave::ResolvedModule resolved = z3wire::weave::Resolve(module);

  // Create output directory.
  std::filesystem::create_directories(args.output_dir);

  std::string prefix = resolved.file_prefix;

  // Generate proto.
  std::string proto_output = z3wire::weave::EmitProto(resolved);
  std::string proto_path = args.output_dir + "/" + prefix + ".proto";
  std::ofstream proto_file(proto_path);
  proto_file << proto_output;
  proto_file.close();
  std::cout << "Generated " << proto_path << "\n";

  // Generate header.
  std::string header_output =
      z3wire::weave::EmitHeader(resolved, prefix + ".pb.h");
  std::string header_path = args.output_dir + "/" + prefix + ".h";
  std::ofstream header_file(header_path);
  header_file << header_output;
  header_file.close();
  std::cout << "Generated " << header_path << "\n";

  return 0;
}
