# Pre-setup
cmake_minimum_required(VERSION 3.19.0)

# Project data
project(subnetooor VERSION 0.0.1 DESCRIPTION "")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
message("C++ Standard: ${CMAKE_CXX_STANDARD}")
message("C++ Standard is required: ${CMAKE_CXX_STANDARD_REQUIRED}")
message("C++ extensions: ${CMAKE_CXX_EXTENSIONS}")
message("Using PIC: ${CMAKE_POSITION_INDEPENDENT_CODE}")

# Include Protobuf
include(FindProtobuf)
include(cmake/FindGRPC.cmake)
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Generate gRPC files
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.h"
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.proto"
)
add_custom_target(gen-grpc ALL DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.h"
)

# Compile and link the protobuf files
protobuf_generate_cpp(PROTO_VM_SRCS PROTO_VM_HDRS proto/vm.proto)
protobuf_generate_cpp(PROTO_METRICS_SRCS PROTO_METRICS_HDRS proto/metrics.proto)

# Compile and link the executable
add_executable(storageVM src/main.cpp src/main.h
  ${PROTO_VM_SRCS} ${PROTO_VM_HDRS} ${PROTO_METRICS_SRCS} ${PROTO_METRICS_HDRS}
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.h"
)
add_dependencies(storageVM gen-grpc)
target_link_libraries(storageVM ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES})

