find_package(absl CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)

# --- Proto library ---
# protobuf_generate puts rdl.pb.h into CMAKE_CURRENT_BINARY_DIR
# (i.e., build/z3wire_weave/). Source files include "z3wire_weave/rdl.pb.h",
# so expose PROJECT_BINARY_DIR so the z3wire_weave/ prefix resolves.
add_library(z3wire_rdl_proto STATIC)
target_link_libraries(z3wire_rdl_proto PUBLIC protobuf::libprotobuf)
target_include_directories(z3wire_rdl_proto PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>")
protobuf_generate(TARGET z3wire_rdl_proto PROTOS rdl.proto)

# --- Resolver ---
add_library(z3wire_resolver resolver.cc resolver.h)
target_link_libraries(z3wire_resolver
    PUBLIC z3wire_rdl_proto
    PRIVATE absl::str_format)
target_include_directories(z3wire_resolver PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")

# --- Emit proto ---
add_library(z3wire_emit_proto emit_proto.cc emit_proto.h)
target_link_libraries(z3wire_emit_proto
    PUBLIC z3wire_resolver
    PRIVATE absl::strings)
target_include_directories(z3wire_emit_proto PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")

# --- Emit header ---
add_library(z3wire_emit_header emit_header.cc emit_header.h)
target_link_libraries(z3wire_emit_header
    PUBLIC z3wire_resolver
    PRIVATE absl::strings absl::str_format)
target_include_directories(z3wire_emit_header PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")

# --- Weave binary ---
add_executable(weave weave_main.cc)
target_link_libraries(weave PRIVATE
    z3wire_resolver z3wire_emit_proto z3wire_emit_header
    z3wire_rdl_proto protobuf::libprotobuf)
target_include_directories(weave PRIVATE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")

# --- Tests ---
if(BUILD_TESTING)
    add_executable(weave_resolver_test resolver_test.cc)
    target_link_libraries(weave_resolver_test PRIVATE
        z3wire_resolver z3wire_rdl_proto GTest::gtest_main GTest::gmock)
    target_include_directories(weave_resolver_test PRIVATE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")
    add_test(NAME weave_resolver_test COMMAND weave_resolver_test)

    add_executable(weave_emit_proto_test emit_proto_test.cc)
    target_link_libraries(weave_emit_proto_test PRIVATE
        z3wire_emit_proto z3wire_rdl_proto GTest::gtest_main GTest::gmock)
    target_include_directories(weave_emit_proto_test PRIVATE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")
    add_test(NAME weave_emit_proto_test COMMAND weave_emit_proto_test)

    add_executable(weave_emit_header_test emit_header_test.cc)
    target_link_libraries(weave_emit_header_test PRIVATE
        z3wire_emit_header z3wire_resolver z3wire_rdl_proto
        GTest::gtest_main GTest::gmock)
    target_include_directories(weave_emit_header_test PRIVATE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")
    add_test(NAME weave_emit_header_test COMMAND weave_emit_header_test)
endif()
