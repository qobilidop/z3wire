"""Rule for testing that C++ code fails to compile with an expected message."""

load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _cc_compile_fail_test_impl(ctx):
    src = ctx.file.src
    expected_message = ctx.attr.expected_message

    # Get the C++ toolchain and compiler.
    cc_toolchain = ctx.toolchains["@bazel_tools//tools/cpp:toolchain_type"].cc
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )

    cc_compiler = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = "c++-compile",
    )

    # Gather header files from deps and compute include directories.
    # We use the header files' short_path to determine their runfiles
    # location and derive include directories.
    header_files = []
    include_dirs = {}  # Use dict as set to deduplicate.
    for dep in ctx.attr.deps:
        if CcInfo in dep:
            cc_ctx = dep[CcInfo].compilation_context
            for hdr in cc_ctx.headers.to_list():
                header_files.append(hdr)

            # Collect system_includes from CcInfo and map them to
            # runfiles-relative paths.
            for inc in cc_ctx.system_includes.to_list():
                include_dirs[inc] = True
            for inc in cc_ctx.includes.to_list():
                include_dirs[inc] = True
            for inc in cc_ctx.quote_includes.to_list():
                include_dirs[inc] = True

    # Generate the test script. Include paths are resolved at test time
    # using $RUNFILES_DIR, which Bazel sets for test actions.
    script = ctx.actions.declare_file(ctx.label.name + ".sh")
    script_content = """\
#!/usr/bin/env bash
# Deliberately omit -e: the compiler invocation is expected to fail,
# and set -e would exit the script at that point.
set -uo pipefail

# Resolve runfiles directory.
if [[ -z "${{RUNFILES_DIR:-}}" ]]; then
    RUNFILES_DIR="$0.runfiles"
fi

COMPILER="{compiler}"
SRC="$RUNFILES_DIR/_main/{src}"
EXPECTED_MSG='{expected_message}'

# Build include flags. We add _main for workspace headers and resolve
# external repo paths through the runfiles tree.
INCLUDE_FLAGS="-isystem $RUNFILES_DIR/_main"
{extra_include_flags}

STDERR=$("$COMPILER" -std=c++20 -fsyntax-only $INCLUDE_FLAGS "$SRC" 2>&1)
EXIT_CODE=$?

if [ "$EXIT_CODE" -eq 0 ]; then
    echo "FAIL: Compilation succeeded, but it should have failed."
    exit 1
fi

if echo "$STDERR" | grep -qF "$EXPECTED_MSG"; then
    echo "PASS: Compilation failed with expected message."
    exit 0
else
    echo "FAIL: Compilation failed, but expected message not found."
    echo "Expected: $EXPECTED_MSG"
    echo "Actual stderr:"
    echo "$STDERR"
    exit 1
fi
"""

    # Build extra include flags from CcInfo include directories.
    # These paths are exec-root relative, so we need to translate them
    # to runfiles paths. Exec-root paths like
    # "bazel-out/<config>/bin/external/<repo>/<path>" map to
    # "$RUNFILES_DIR/<repo>/<path>".
    extra_lines = []
    for inc in include_dirs.keys():
        if inc == "" or inc == ".":
            # Workspace root, already handled.
            continue

        # External repo path: bazel-out/<config>/bin/external/<repo>/<rest>
        # -> $RUNFILES_DIR/<repo>/<rest>
        parts = inc.split("/")
        if len(parts) >= 4 and parts[0] == "bazel-out" and parts[2] == "bin":
            # bazel-out/<config>/bin/external/<repo>/... or
            # bazel-out/<config>/bin/<repo>/...
            if len(parts) >= 5 and parts[3] == "external":
                runfiles_path = "/".join(parts[4:])
            else:
                runfiles_path = "_main/" + "/".join(parts[3:])
            extra_lines.append(
                'INCLUDE_FLAGS="$INCLUDE_FLAGS -isystem $RUNFILES_DIR/{}"'.format(runfiles_path),
            )
        elif not inc.startswith("bazel-out"):
            # Source-tree relative path.
            extra_lines.append(
                'INCLUDE_FLAGS="$INCLUDE_FLAGS -isystem $RUNFILES_DIR/_main/{}"'.format(inc),
            )

    script_content = script_content.format(
        compiler = cc_compiler,
        src = src.short_path,
        expected_message = expected_message,
        extra_include_flags = "\n".join(extra_lines),
    )

    ctx.actions.write(
        output = script,
        content = script_content,
        is_executable = True,
    )

    # All files needed at test runtime.
    runfiles = ctx.runfiles(files = [src] + header_files)
    runfiles = runfiles.merge_all([
        dep[DefaultInfo].default_runfiles
        for dep in ctx.attr.deps
        if dep[DefaultInfo].default_runfiles
    ])

    # Include the CC toolchain files so the compiler binary is available.
    runfiles = runfiles.merge(
        ctx.runfiles(transitive_files = cc_toolchain.all_files),
    )

    return [DefaultInfo(
        executable = script,
        runfiles = runfiles,
    )]

cc_compile_fail_test = rule(
    implementation = _cc_compile_fail_test_impl,
    test = True,
    attrs = {
        "src": attr.label(
            allow_single_file = [".cc", ".cpp"],
            mandatory = True,
        ),
        "deps": attr.label_list(
            providers = [CcInfo],
        ),
        "expected_message": attr.string(
            mandatory = True,
        ),
    },
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    fragments = ["cpp"],
)
