"""Weave: generate C++ and proto files from Z3Wire RDL descriptions."""

import argparse
import os

from google.protobuf import text_format

from z3wire.weave import rdl_pb2
from z3wire.weave.emit_header import emit_header
from z3wire.weave.emit_proto import emit_proto
from z3wire.weave.resolver import resolve


def main():
    parser = argparse.ArgumentParser(
        description="Generate C++ and proto files from Z3Wire RDL descriptions."
    )
    parser.add_argument("--input", required=True, help="Path to .txtpb file")
    parser.add_argument("--output_dir", required=True, help="Output directory")
    args = parser.parse_args()

    # Read and parse input
    with open(args.input) as f:
        text = f.read()

    module = rdl_pb2.Module()
    text_format.Parse(text, module)

    # Resolve
    resolved = resolve(module)

    # Output filenames from file_prefix
    prefix = resolved.file_prefix

    # Generate outputs
    os.makedirs(args.output_dir, exist_ok=True)

    proto_output = emit_proto(resolved)
    proto_path = os.path.join(args.output_dir, f"{prefix}.proto")
    with open(proto_path, "w") as f:
        f.write(proto_output)
    print(f"Generated {proto_path}")

    header_output = emit_header(resolved, proto_header=f"{prefix}.pb.h")
    header_path = os.path.join(args.output_dir, f"{prefix}.h")
    with open(header_path, "w") as f:
        f.write(header_output)
    print(f"Generated {header_path}")


if __name__ == "__main__":
    main()
