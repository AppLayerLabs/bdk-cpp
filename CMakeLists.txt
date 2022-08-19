# Pre-setup
cmake_minimum_required(VERSION 3.19.0)

include(cmake/cable/bootstrap.cmake)
include(CableBuildInfo)
include(CableBuildType)
include(GNUInstallDirs)
cable_set_build_type(DEFAULT Debug CONFIGURATION_TYPES Debug;Release;RelWithDebInfo)

# Map current configuration to configurations of imported targets.
set(CMAKE_MAP_IMPORTED_CONFIG_Debug Debug)
set(CMAKE_MAP_IMPORTED_CONFIG_RelWithDebInfo Debug)

# Project data
project(subnetooor VERSION 0.0.1 DESCRIPTION "Sparq subnet")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-O2")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a") # "ZLIB_USE_STATIC_LIBS" was added in 3.24
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy
set(LEVELDB_USE_STATIC_LIBS ON)
set(BUILD_TESTS ON CACHE BOOL "Build helper unit testing program")
set(USE_LINT OFF CACHE BOOL "Run linter on compile (clang-tidy)")
if(USE_LINT)
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.;-checks=-*,abseil-*,boost-*,bugprone-*,cert-*,clang-analyzer-*,concurrency-*,cppcoreguidelines-*,hicpp-*,misc-*,modernize-*,performance-*,portability-*,readability-*")
endif()
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Standard is required: ${CMAKE_CXX_STANDARD_REQUIRED}")
message(STATUS "C++ extensions: ${CMAKE_CXX_EXTENSIONS}")
message(STATUS "Using PIC: ${CMAKE_POSITION_INDEPENDENT_CODE}")
message(STATUS "Find libs with suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")
message("Building tests: ${BUILD_TESTS}")

cable_add_buildinfo_library(PROJECT_NAME subnetooor)

# System package configs (built-in)
set(Boost_USE_STATIC_LIBS ON)
set(OPENSSL_USE_STATIC_LIBS ON)
set(Protobuf_USE_STATIC_LIBS ON)

# Find system packages (built-in)
find_package(Threads)
find_package(Boost 1.74.0 REQUIRED COMPONENTS
  chrono filesystem program_options system thread nowide
)
find_package(OpenSSL 1.1.1 REQUIRED)
find_package(Protobuf 3.12 REQUIRED)
find_package(ZLIB REQUIRED)

# Find system packages (custom)
find_package(Absl REQUIRED) # Built-in is hardcoded to SHARED, this one to STATIC
find_package(Cares REQUIRED)
find_package(CryptoPP 8.2.0 REQUIRED)
find_package(GRPC REQUIRED)
find_package(Scrypt REQUIRED)
find_package(LevelDB REQUIRED)

# Add external modules
include(cmake/ProjectBoostCertify.cmake)
include(cmake/ProjectEthash.cmake)
include(cmake/ProjectSecp256k1.cmake) # Itamar's fork which includes secp256k1_sha256.h

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
file(GLOB DEVCORE_HEADERS "src/libs/devcore/*.h")
file(GLOB DEVCORE_SOURCES "src/libs/devcore/*.cpp")
file(GLOB DEVCRYPTO_HEADERS "src/libs/devcrypto/*.h")
file(GLOB DEVCRYPTO_SOURCES "src/libs/devcrypto/*.cpp")
file(GLOB ETHCORE_HEADERS "src/libs/ethcore/*.h")
file(GLOB ETHCORE_SOURCES "src/libs/ethcore/*.cpp")

add_library(aleth STATIC
  ${DEVCORE_HEADERS} ${DEVCORE_SOURCES}
  ${DEVCRYPTO_HEADERS} ${DEVCRYPTO_SOURCES}
  ${ETHCORE_HEADERS} ${ETHCORE_SOURCES}
)
target_include_directories(aleth PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(aleth PRIVATE
  cryptopp scrypt Secp256k1 Ethash ${ETHASH_BYPRODUCTS}
)

# Generate gRPC files
# Headers/sources are always cleaned at configure stage
# so they can be regenerated at build stage
file(REMOVE
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.h"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.h"
  COMMAND "protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.h"
  COMMAND "protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.h"
  COMMAND "protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.h"
  COMMAND "protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/metrics.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/metrics.grpc.pb.h"
  COMMAND "protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/metrics.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.h"
  COMMAND "protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.h"
  COMMAND "protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.h"
  COMMAND "protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.proto"
)

# Compile and link the protobuf files
protobuf_generate_cpp(PROTO_VM_SRCS PROTO_VM_HDRS proto/vm.proto)
protobuf_generate_cpp(PROTO_METRICS_SRCS PROTO_METRICS_HDRS proto/metrics.proto)
protobuf_generate_cpp(PROTO_ALIASREADER_SRCS PROTO_ALIASREADER_HDRS proto/aliasreader.proto)
protobuf_generate_cpp(PROTO_APPSENDER_SRCS PROTO_APPSENDER_HDRS proto/appsender.proto)
protobuf_generate_cpp(PROTO_KEYSTORE_SRCS PROTO_KEYSTORE_HDRS proto/keystore.proto)
protobuf_generate_cpp(PROTO_MESSENGER_SRCS PROTO_MESSENGER_HDRS proto/messenger.proto)
protobuf_generate_cpp(PROTO_SHAREDMEMORY_SRCS PROTO_SHAREDMEMORY_HDRS proto/sharedmemory.proto)
protobuf_generate_cpp(PROTO_RPCDB_SRCS PROTO_RPCDB_HDRS proto/rpcdb.proto)

add_custom_target(gen-grpc ALL DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.h"
)

#file(GLOB SUBNETOOOR_HDRS "src/*.h")
#file(GLOB SUBNETOOOR_SRCS "src/*.cpp")

# Set project headers and sources
set(SUBNETOOOR_HDRS
  src/contract/abi.h
  src/contract/contract.h
  src/contract/erc20.h
  src/contract/erc721.h
  src/core/block.h
  src/core/chainHead.h
  src/core/db.h
  src/core/secp256k1Wrapper.h
  src/core/state.h
  src/core/subnet.h
  src/core/transaction.h
  src/core/utils.h
  src/libs/keccak.hpp
  src/net/grpcclient.h
  src/net/grpcserver.h
  src/net/httpserver.h
)

set(SUBNETOOOR_SRCS
  src/contract/erc20.cpp
  src/core/block.cpp
  src/core/chainHead.cpp
  src/core/db.cpp
  src/core/secp256k1Wrapper.cpp
  src/core/state.cpp
  src/core/subnet.cpp
  src/core/subnetRPC.cpp
  src/core/transaction.cpp
  src/core/utils.cpp
  src/net/grpcclient.cpp
  src/net/grpcserver.cpp
  src/net/httpserver.cpp
  src/main.cpp
)

#message(STATUS ${SUBNETOOOR_HDRS})
#message(STATUS ${SUBNETOOOR_SRCS})

# Compile and link the executable
add_executable(subnetooord
  ${SUBNETOOOR_SRCS} ${SUBNETOOOR_HDRS}
  ${PROTO_VM_SRCS} ${PROTO_VM_HDRS}
  ${PROTO_METRICS_SRCS} ${PROTO_METRICS_HDRS}
  ${PROTO_METRICS_SRCS} ${PROTO_METRICS_HDRS}
  ${PROTO_ALIASREADER_SRCS} ${PROTO_ALIASREADER_HDRS}
  ${PROTO_APPSENDER_SRCS} ${PROTO_APPSENDER_HDRS}
  ${PROTO_KEYSTORE_SRCS} ${PROTO_KEYSTORE_HDRS}
  ${PROTO_MESSENGER_SRCS} ${PROTO_MESSENGER_HDRS}
  ${PROTO_SHAREDMEMORY_SRCS} ${PROTO_SHAREDMEMORY_HDRS}
  ${PROTO_RPCDB_SRCS} ${PROTO_RPCDB_HDRS}
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.h"
)
add_dependencies(subnetooord gen-grpc)

# Add include folder and link libraries to the executable
target_include_directories(subnetooord PRIVATE aleth)
target_link_libraries(subnetooord aleth
  ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} ${LEVELDB_LIBRARY}
  ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
  absl::flags Secp256k1
)

set_target_properties(subnetooord PROPERTIES COMPILE_FLAGS "-DIS_LOCAL_TESTS=0")

# Build helper test program
if(BUILD_TESTS)
  # Required sources for test from src/ (different folder)
  set(SUBNETOOOR_TESTS_HDRS
    src/core/block.h
    src/core/chainHead.h
    src/core/db.h
    src/core/secp256k1Wrapper.h
    src/core/state.h
    src/core/subnet.h
    src/core/transaction.h
    src/core/utils.h
    src/libs/keccak.hpp
    src/net/grpcclient.h
    src/net/grpcserver.h
    src/net/httpserver.h
  )
  set(SUBNETOOOR_TESTS_SRCS
    src/core/block.cpp
    src/core/chainHead.cpp
    src/core/db.cpp
    src/core/secp256k1Wrapper.cpp
    src/core/state.cpp
    src/core/subnet.cpp
    src/core/subnetRPC.cpp
    src/core/transaction.cpp
    src/core/utils.cpp
    src/net/grpcclient.cpp
    src/net/grpcserver.cpp
    src/net/httpserver.cpp
  )
  add_executable(subnetooord-tests
    "tests/main.cpp"
    "tests/tests.h"
    "tests/transaction.cpp"
    "tests/utils.cpp"
    "tests/state.cpp"
    "tests/chainHead.cpp"
    ${SUBNETOOOR_TESTS_HDRS}
    ${SUBNETOOOR_TESTS_SRCS}
    ${PROTO_VM_SRCS} ${PROTO_VM_HDRS}
    ${PROTO_METRICS_SRCS} ${PROTO_METRICS_HDRS}
    ${PROTO_METRICS_SRCS} ${PROTO_METRICS_HDRS}
    ${PROTO_ALIASREADER_SRCS} ${PROTO_ALIASREADER_HDRS}
    ${PROTO_APPSENDER_SRCS} ${PROTO_APPSENDER_HDRS}
    ${PROTO_KEYSTORE_SRCS} ${PROTO_KEYSTORE_HDRS}
    ${PROTO_MESSENGER_SRCS} ${PROTO_MESSENGER_HDRS}
    ${PROTO_SHAREDMEMORY_SRCS} ${PROTO_SHAREDMEMORY_HDRS}
    ${PROTO_RPCDB_SRCS} ${PROTO_RPCDB_HDRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.h"
  )
  target_include_directories(subnetooord-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(subnetooord-tests aleth
    ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} ${LEVELDB_LIBRARY}
    ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
    absl::flags Secp256k1
  )
  set_target_properties(subnetooord-tests PROPERTIES COMPILE_FLAGS "-DIS_LOCAL_TESTS=1")
endif()

