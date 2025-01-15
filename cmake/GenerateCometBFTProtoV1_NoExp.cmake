# NoExp:
# debugging grpc server shutdown crashes
# remove        ###--experimental_allow_proto3_optional
# remove lite

# TODO: Decide on protoc --cpp-out=lite: when everything is working

# Define the root proto directory
set(PROTO_ROOT_DIR "${CMAKE_SOURCE_DIR}/proto")

# Log the values of all relevant variables
message(STATUS "PROTO_ROOT_DIR: ${PROTO_ROOT_DIR}")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

# Find all .proto files in the proto/ directory recursively and echo them.
# Equivalent to `proto$ find . -name "*.proto" -print`
file(GLOB_RECURSE PROTO_FILES "${PROTO_ROOT_DIR}/*.proto")
message(STATUS "Proto files found:")
foreach(PROTO_FILE ${PROTO_FILES})
  message(STATUS "  ${PROTO_FILE}")
endforeach()

# Initialize lists to hold generated files
set(PROTO_SOURCES)
set(PROTO_HEADERS)

# Create the list of .proto generated sources (.pb.cc)
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/abci/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/abci/v1/service.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/libs/bits/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/state/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/services/block/v1/block_service.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/services/block/v1/block.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/services/version/v1/version_service.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/services/version/v1/version.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/services/block_results/v1/block_results.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/services/block_results/v1/block_results_service.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/services/pruning/v1/service.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/services/pruning/v1/pruning.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/crypto/v1/keys.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/crypto/v1/proof.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/privval/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/types/v1/block.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/types/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/types/v1/events.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/types/v1/params.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/types/v1/validator.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/types/v1/canonical.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/types/v1/evidence.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/p2p/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/p2p/v1/pex.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/p2p/v1/conn.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/statesync/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/blocksync/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/mempool/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/store/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/version/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/consensus/v1/types.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/cometbft/consensus/v1/wal.pb.cc")
list(APPEND PROTO_SOURCES "${PROTO_ROOT_DIR}/gogoproto/gogo.pb.cc")

# Create the list of .proto generated headers (.pb.h)
# Same as above (they always generate in pairs), we just change the suffix
foreach(source ${PROTO_SOURCES})
  string(REPLACE ".cc" ".h" header ${source})
  list(APPEND PROTO_HEADERS ${header})
endforeach()
list(APPEND PROTO_SOURCES ${PROTO_HEADERS})

# Generate the .proto sources and headers themselves
# TODO: this could be further optimized with a for loop maybe?
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/abci/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/abci/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/abci/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/abci/v1/service.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/abci/v1/service.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/abci/v1/service.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/libs/bits/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/libs/bits/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/libs/bits/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/state/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/state/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/state/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/services/block/v1/block_service.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/services/block/v1/block_service.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/services/block/v1/block_service.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/services/block/v1/block.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/services/block/v1/block.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/services/block/v1/block.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/services/version/v1/version_service.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/services/version/v1/version_service.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/services/version/v1/version_service.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/services/version/v1/version.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/services/version/v1/version.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/services/version/v1/version.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/services/block_results/v1/block_results.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/services/block_results/v1/block_results.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/services/block_results/v1/block_results.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/services/block_results/v1/block_results_service.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/services/block_results/v1/block_results_service.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/services/block_results/v1/block_results_service.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/services/pruning/v1/service.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/services/pruning/v1/service.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/services/pruning/v1/service.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/services/pruning/v1/pruning.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/services/pruning/v1/pruning.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/services/pruning/v1/pruning.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/consensus/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/consensus/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/consensus/v1/types.proto"

       # Run a fix.sh script that should already be in the same dir as the generated .cc and .h files
       # This script fixes the vote / has_vote protoc C++ code generator collisions
       COMMAND ${CMAKE_COMMAND} -E echo "Running fix.sh script to apply sed substitutions"
       COMMAND bash "${PROTO_ROOT_DIR}/cometbft/consensus/v1/fix.sh"
       DEPENDS "${PROTO_ROOT_DIR}/cometbft/consensus/v1/types.proto"
       COMMENT "Generating C++ source files and applying fixes with fix.sh"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/consensus/v1/wal.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/consensus/v1/wal.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/consensus/v1/wal.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/crypto/v1/keys.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/crypto/v1/keys.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/crypto/v1/keys.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/crypto/v1/proof.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/crypto/v1/proof.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/crypto/v1/proof.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/privval/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/privval/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/privval/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/types/v1/block.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/types/v1/block.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/types/v1/block.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/types/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/types/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/types/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/types/v1/events.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/types/v1/events.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/types/v1/events.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/types/v1/params.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/types/v1/params.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/types/v1/params.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/types/v1/validator.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/types/v1/validator.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/types/v1/validator.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/types/v1/canonical.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/types/v1/canonical.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/types/v1/canonical.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/types/v1/evidence.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/types/v1/evidence.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/types/v1/evidence.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/p2p/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/p2p/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/p2p/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/p2p/v1/pex.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/p2p/v1/pex.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/p2p/v1/pex.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/p2p/v1/conn.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/p2p/v1/conn.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/p2p/v1/conn.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/statesync/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/statesync/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/statesync/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/blocksync/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/blocksync/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/blocksync/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/mempool/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/mempool/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/mempool/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/store/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/store/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/store/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/cometbft/version/v1/types.pb.cc"
         "${PROTO_ROOT_DIR}/cometbft/version/v1/types.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/cometbft/version/v1/types.proto"
)
add_custom_command(
  OUTPUT "${PROTO_ROOT_DIR}/gogoproto/gogo.pb.cc"
         "${PROTO_ROOT_DIR}/gogoproto/gogo.pb.h"
  COMMAND protoc
  ARGS --cpp_out=lite:${PROTO_ROOT_DIR}
       --proto_path=${PROTO_ROOT_DIR}
       ###--experimental_allow_proto3_optional
       "${PROTO_ROOT_DIR}/gogoproto/gogo.proto"
)

# Create a static library that holds all of the generated proto files
add_library(gen-proto-grpc STATIC ${PROTO_SOURCES} ${PROTO_HEADERS})

# Include the proto directories in it so the subsequent includes work during compilation
target_include_directories(gen-proto-grpc PUBLIC "${PROTO_ROOT_DIR}")

# Link the proto files against the required libraries (e.g. absl).
target_link_libraries(gen-proto-grpc PUBLIC
  absl::check     # helloworld generated lib
  absl::absl_log  # route-guide generated lib
  ${_REFLECTION}  # these three are the same across both the helloworld and route-guide examples
  ${_PROTOBUF_LIBPROTOBUF}
)

