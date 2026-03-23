// Weave: generate C++ and proto files from Z3Wire wire spec descriptions.

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "google/protobuf/text_format.h"
#include "z3wire_weave/emit_header.h"
#include "z3wire_weave/emit_proto.h"
#include "z3wire_weave/wire_spec.pb.h"
#include "z3wire_weave/resolver.h"

ABSL_FLAG(std::string, input, "", "Path to wire spec textproto file.");
ABSL_FLAG(std::string, output_dir, "", "Directory for generated files.");
ABSL_FLAG(std::string, include_prefix, "",
          "Path prefix for the generated proto #include.");

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);

  std::string input = absl::GetFlag(FLAGS_input);
  std::string output_dir = absl::GetFlag(FLAGS_output_dir);
  std::string include_prefix = absl::GetFlag(FLAGS_include_prefix);

  if (input.empty() || output_dir.empty()) {
    std::cerr << "Error: --input and --output_dir are required.\n";
    return 1;
  }

  // Read input file.
  std::ifstream input_file(input);
  if (!input_file) {
    std::cerr << "Error: cannot open " << input << "\n";
    return 1;
  }
  std::ostringstream ss;
  ss << input_file.rdbuf();
  std::string text = ss.str();

  // Parse textproto.
  z3wire_weave::WireSpec module;
  if (!google::protobuf::TextFormat::ParseFromString(text, &module)) {
    std::cerr << "Error: failed to parse " << input << "\n";
    return 1;
  }

  // Resolve.
  z3wire_weave::ResolvedModule resolved = z3wire_weave::Resolve(module);

  // Create output directory.
  std::filesystem::create_directories(output_dir);

  std::string prefix = resolved.file_prefix;

  // Generate proto.
  std::string proto_output = z3wire_weave::EmitProto(resolved);
  std::string proto_path = output_dir + "/" + prefix + ".proto";
  std::ofstream proto_file(proto_path);
  proto_file << proto_output;
  proto_file.close();
  std::cout << "Generated " << proto_path << "\n";

  // Generate header.
  std::string proto_header = include_prefix.empty()
                                 ? prefix + ".pb.h"
                                 : include_prefix + "/" + prefix + ".pb.h";
  std::string header_output =
      z3wire_weave::EmitHeader(resolved, proto_header);
  std::string header_path = output_dir + "/" + prefix + ".h";
  std::ofstream header_file(header_path);
  header_file << header_output;
  header_file.close();
  std::cout << "Generated " << header_path << "\n";

  return 0;
}
