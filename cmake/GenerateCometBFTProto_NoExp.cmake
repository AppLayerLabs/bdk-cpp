# NoExp:
# debugging grpc server shutdown crashes
# remove        ###--experimental_allow_proto3_optional
# keep lite for now (TODO: Decide on protoc --cpp_out=lite: when everything is working)

# Define the root proto directory
set(PROTO_ROOT_DIR "${CMAKE_SOURCE_DIR}/proto")

# Try to use CMake-found protoc, else fall back to 'protoc' on PATH (keeps your style)
set(PROTOC_CMD protoc)

# Log the values of all relevant variables
message(STATUS "PROTO_ROOT_DIR: ${PROTO_ROOT_DIR}")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "Using protoc command: ${PROTOC_CMD}")

# Find all .proto files in the proto/ directory recursively and echo them.
file(GLOB_RECURSE PROTO_FILES "${PROTO_ROOT_DIR}/*.proto")
message(STATUS "Proto files found:")
foreach(PROTO_FILE ${PROTO_FILES})
    message(STATUS "  ${PROTO_FILE}")
endforeach()

# Initialize lists to hold generated files
set(PROTO_SOURCES)
set(PROTO_HEADERS)

# ---------------------------------------------------------------------------
# Updated list of generated sources/headers matching your CURRENT tree
# (removed tendermint/services/*, abci/service.proto, pruning/*, etc.)
# Added tendermint/rpc/grpc/types.proto
# ---------------------------------------------------------------------------

# .pb.cc
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/abci/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/blocksync/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/consensus/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/consensus/wal.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/crypto/keys.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/crypto/proof.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/libs/bits/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/mempool/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/p2p/conn.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/p2p/pex.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/p2p/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/privval/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/rpc/grpc/types.pb.cc")   # NEW
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/state/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/statesync/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/store/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/types/block.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/types/canonical.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/types/events.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/types/evidence.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/types/params.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/types/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/types/validator.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/tendermint/version/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/gogoproto/gogo.pb.cc")

# Create the list of .proto generated headers (.pb.h)
foreach(source ${PROTO_SOURCES})
    string(REPLACE ".cc" ".h" header ${source})
    list(APPEND PROTO_HEADERS ${header})
endforeach()
list(APPEND PROTO_SOURCES ${PROTO_HEADERS})

# ---------------------------------------------------------------------------
# Codegen commands (same style as yours; only file list changed + rpc/grpc)
# ---------------------------------------------------------------------------

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/abci/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/abci/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/abci/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/blocksync/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/blocksync/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/blocksync/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/consensus/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/consensus/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/consensus/types.proto"
        # Fix hasVote
        COMMAND bash "${PROTO_ROOT_DIR}/tendermint/consensus/fix.sh"
        DEPENDS "${PROTO_ROOT_DIR}/tendermint/consensus/types.proto"
        COMMENT "Generating C++ source files and applying fixes with fix.sh"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/consensus/wal.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/consensus/wal.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/consensus/wal.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/crypto/keys.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/crypto/keys.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/crypto/keys.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/crypto/proof.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/crypto/proof.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/crypto/proof.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/libs/bits/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/libs/bits/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/libs/bits/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/mempool/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/mempool/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/mempool/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/p2p/conn.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/p2p/conn.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/p2p/conn.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/p2p/pex.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/p2p/pex.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/p2p/pex.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/p2p/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/p2p/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/p2p/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/privval/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/privval/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/privval/types.proto"
)

# NEW: tendermint/rpc/grpc/types.proto
add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/rpc/grpc/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/rpc/grpc/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/rpc/grpc/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/state/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/state/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/state/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/statesync/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/statesync/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/statesync/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/store/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/store/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/store/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/types/block.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/types/block.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/types/block.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/types/canonical.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/types/canonical.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/types/canonical.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/types/events.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/types/events.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/types/events.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/types/evidence.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/types/evidence.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/types/evidence.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/types/params.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/types/params.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/types/params.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/types/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/types/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/types/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/types/validator.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/types/validator.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/types/validator.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/tendermint/version/types.pb.cc"
        "${PROTO_ROOT_DIR}/tendermint/version/types.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/tendermint/version/types.proto"
)

add_custom_command(
        OUTPUT "${PROTO_ROOT_DIR}/gogoproto/gogo.pb.cc"
        "${PROTO_ROOT_DIR}/gogoproto/gogo.pb.h"
        COMMAND ${PROTOC_CMD}
        ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
        --proto_path=${PROTO_ROOT_DIR}
        "${PROTO_ROOT_DIR}/gogoproto/gogo.proto"
)

# Create a static library that holds all of the generated proto files
add_library(gen-proto-grpc STATIC ${PROTO_SOURCES} ${PROTO_HEADERS})

# Include the proto directories so subsequent includes work during compilation
target_include_directories(gen-proto-grpc PUBLIC "${PROTO_ROOT_DIR}")

# Link the proto files against the required libraries (e.g. absl).
target_link_libraries(gen-proto-grpc PUBLIC
        absl::check
        absl::absl_log
        ${_REFLECTION}
        ${_PROTOBUF_LIBPROTOBUF}
)
