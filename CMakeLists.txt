# Pre-setup
cmake_minimum_required(VERSION 3.19.0)

include(cmake/cable/bootstrap.cmake)
include(CableBuildInfo)
include(CableBuildType)
include(GNUInstallDirs)
cable_set_build_type(DEFAULT Release CONFIGURATION_TYPES Debug;Release;RelWithDebInfo)

# Map current configuration to configurations of imported targets.
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release)
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)

# Get Hunter
include("cmake/HunterGate.cmake")
HunterGate(
  URL "https://github.com/cpp-pm/hunter/archive/v0.23.288.tar.gz"
  SHA1 "6c9b2bc606d86ae31f96a62fc68f0a593024815b"
  LOCAL # Load config from `${CMAKE_CURRENT_LIST_DIR}/cmake/Hunter/config.cmake`
)

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

cable_add_buildinfo_library(PROJECT_NAME subnetooor)

# Add Hunter packages
hunter_add_package(cryptopp)
find_package(cryptopp CONFIG REQUIRED)
hunter_add_package(libscrypt)
find_package(libscrypt CONFIG REQUIRED)

# Add external modules
include(cmake/ProjectEthash.cmake)
include(cmake/ProjectSecp256k1.cmake)
include(cmake/ProjectToolbox.cmake)
include(ProjectSnappy)
include(ProjectLevelDB)
find_package(Threads)

# Include Protobuf
include(FindProtobuf)
include(FindZLIB)
include(cmake/FindGRPC.cmake)
find_package(Protobuf REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(gRPC REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options regex thread system chrono filesystem)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${Protobuf_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Include headers and libs
include_directories(
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/build"
  "${CMAKE_SOURCE_DIR}/build/deps/include"
)
link_directories(
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/build"
  "${CMAKE_SOURCE_DIR}/build/deps/lib"
)

# Organize, compile and link aleth libs
file(GLOB DEVCORE_HEADERS "include/web3cpp/devcore/*.h")
file(GLOB DEVCORE_SOURCES "src/libs/devcore/*.cpp")
file(GLOB DEVCRYPTO_HEADERS "include/web3cpp/devcrypto/*.h")
file(GLOB DEVCRYPTO_SOURCES "src/libs/devcrypto/*.cpp")
file(GLOB ETHCORE_HEADERS "include/web3cpp/ethcore/*.h")
file(GLOB ETHCORE_SOURCES "src/libs/ethcore/*.cpp")

add_library(aleth STATIC
  ${DEVCORE_HEADERS} ${DEVCORE_SOURCES}
  ${DEVCRYPTO_HEADERS} ${DEVCRYPTO_SOURCES}
  ${ETHCORE_HEADERS} ${ETHCORE_SOURCES}
)
target_include_directories(aleth PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(aleth PRIVATE
  cryptopp-static libscrypt::scrypt Secp256k1 toolbox ${ETHASH_BYPRODUCTS} Ethash
)

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
file(GLOB SUBNETOOOR_HEADERS "src/*.h")
file(GLOB SUBNETOOOR_SOURCES "src/*.cpp")

add_executable(subnetooord ${SUBNETOOOR_HEADERS} ${SUBNETOOOR_SOURCES}
  ${PROTO_VM_SRCS} ${PROTO_VM_HDRS} ${PROTO_METRICS_SRCS} ${PROTO_METRICS_HDRS}
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.h"
)
add_dependencies(subnetooord gen-grpc)
# Add include folder to subnetooor
target_include_directories(subnetooord PRIVATE aleth)
target_link_libraries(subnetooord aleth ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} gRPC::grpc++ ${Boost_LIBRARIES} LevelDB snappy)

