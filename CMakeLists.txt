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

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
	cmake_policy(SET CMP0135 NEW)
endif()

# Project data
project(subnetooor VERSION 0.0.1 DESCRIPTION "Sparq subnet")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-O2")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a") # "ZLIB_USE_STATIC_LIBS" was added in 3.24
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy
set(LEVELDB_USE_STATIC_LIBS ON)
set(BUILD_TESTS ON CACHE BOOL "Build helper unit testing program")
set(BUILD_AVALANCHEGO OFF CACHE BOOL "Build with AvalancheGo wrapping")
set(BUILD_TOOLS OFF CACHE BOOL "Build tools related to subnet")
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
message("Building avalanchego: ${BUILD_AVALANCHEGO}")

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
find_package(ZLIB REQUIRED)

# Find system packages (custom)
find_package(CryptoPP 8.2.0 REQUIRED)
find_package(Scrypt REQUIRED)
find_package(LevelDB REQUIRED)

# Add external modules
include(cmake/ProjectBoostCertify.cmake)
include(cmake/ProjectEthash.cmake)
include(cmake/ProjectSecp256k1.cmake) # Bitcoin core fast implementation

# Build catch2 to a library.
add_library(catch2
  ${CMAKE_SOURCE_DIR}/src/libs/catch2/catch_amalgamated.hpp
  ${CMAKE_SOURCE_DIR}/src/libs/catch2/catch_amalgamated.cpp
)

target_include_directories(catch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/catch2)

# Check compiler variable sizes
include(cmake/CheckSizes.cmake)

if(BUILD_AVALANCHEGO)
  # Add avalanchego wrapper dependencies.
  find_package(Absl REQUIRED) # Built-in is hardcoded to SHARED, this one to STATIC
  find_package(Cares REQUIRED)
  find_package(Protobuf 3.12 REQUIRED)
  find_package(GRPC REQUIRED)
endif()

# Include headers and libs
include_directories(
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/src/libs"
  "${CMAKE_SOURCE_DIR}/build"
  "${CMAKE_SOURCE_DIR}/build/deps/include"
)

link_directories(
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/src/libs"
  "${CMAKE_SOURCE_DIR}/build"
  "${CMAKE_SOURCE_DIR}/build/deps/lib"
)

# TODO: change src/ to src/
# Organize, compile and link subnetooor libs
add_subdirectory(src/contract)
add_subdirectory(src/core)
add_subdirectory(src/net)
add_subdirectory(src/utils)
add_subdirectory(tests/)

if(BUILD_AVALANCHEGO)
  # Generate gRPC files
  # Headers/sources are always cleaned at configure stage
  # so they can be regenerated at build stage
  file(REMOVE
    "${CMAKE_SOURCE_DIR}/proto/vm.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/aliasreader.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/appsender.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/keystore.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/messenger.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/sharedmemory.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/rpcdb.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/vm.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/aliasreader.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/appsender.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/keystore.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/messenger.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/sharedmemory.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/rpcdb.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/vm.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/aliasreader.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/appsender.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/keystore.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/messenger.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/sharedmemory.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/rpcdb.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/vm.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/aliasreader.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/appsender.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/keystore.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/messenger.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/sharedmemory.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/rpcdb.pb.h"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/aliasreader.grpc.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/aliasreader.grpc.pb.h"
    COMMAND "protoc"
    ARGS --grpc_out="${CMAKE_SOURCE_DIR}/proto"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/aliasreader.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/appsender.grpc.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/appsender.grpc.pb.h"
    COMMAND "protoc"
    ARGS --grpc_out="${CMAKE_SOURCE_DIR}/proto"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/appsender.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/keystore.grpc.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/keystore.grpc.pb.h"
    COMMAND "protoc"
    ARGS --grpc_out="${CMAKE_SOURCE_DIR}/proto"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/keystore.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/messenger.grpc.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/messenger.grpc.pb.h"
    COMMAND "protoc"
    ARGS --grpc_out="${CMAKE_SOURCE_DIR}/proto"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/messenger.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/metrics.grpc.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/metrics.grpc.pb.h"
    COMMAND "protoc"
    ARGS --grpc_out="${CMAKE_SOURCE_DIR}/proto"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/metrics.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/sharedmemory.grpc.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/sharedmemory.grpc.pb.h"
    COMMAND "protoc"
    ARGS --grpc_out="${CMAKE_SOURCE_DIR}/proto"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/sharedmemory.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/rpcdb.grpc.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/rpcdb.grpc.pb.h"
    COMMAND "protoc"
    ARGS --grpc_out="${CMAKE_SOURCE_DIR}/proto"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/rpcdb.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/vm.grpc.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/vm.grpc.pb.h"
    COMMAND "protoc"
    ARGS --grpc_out="${CMAKE_SOURCE_DIR}/proto"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         --experimental_allow_proto3_optional
         "${CMAKE_SOURCE_DIR}/proto/vm.proto"
  )

  # Protobuf PROTOBUF_GENERATE_CPP does NOT work with --experimental_allow_proto3_optional
  # requiring us to go over protobuf files with add_custom_command

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/aliasreader.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/aliasreader.pb.h"
    COMMAND "protoc"
    ARGS --cpp_out="${CMAKE_SOURCE_DIR}/proto"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/aliasreader.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/appsender.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/appsender.pb.h"
    COMMAND "protoc"
    ARGS --cpp_out="${CMAKE_SOURCE_DIR}/proto"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/appsender.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/keystore.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/keystore.pb.h"
    COMMAND "protoc"
    ARGS --cpp_out="${CMAKE_SOURCE_DIR}/proto"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/keystore.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/messenger.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/messenger.pb.h"
    COMMAND "protoc"
    ARGS --cpp_out="${CMAKE_SOURCE_DIR}/proto"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/messenger.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/metrics.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/metrics.pb.h"
    COMMAND "protoc"
    ARGS --cpp_out="${CMAKE_SOURCE_DIR}/proto"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/metrics.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/sharedmemory.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/sharedmemory.pb.h"
    COMMAND "protoc"
    ARGS --cpp_out="${CMAKE_SOURCE_DIR}/proto"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/sharedmemory.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/rpcdb.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/rpcdb.pb.h"
    COMMAND "protoc"
    ARGS --cpp_out="${CMAKE_SOURCE_DIR}/proto"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/rpcdb.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/metrics.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/metrics.pb.h"
    COMMAND "protoc"
    ARGS --cpp_out="${CMAKE_SOURCE_DIR}/proto"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         "${CMAKE_SOURCE_DIR}/proto/metrics.proto"
  )

  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/proto/vm.pb.cc"
           "${CMAKE_SOURCE_DIR}/proto/vm.pb.h"
    COMMAND "protoc"
    ARGS --cpp_out="${CMAKE_SOURCE_DIR}/proto"
         --proto_path="${CMAKE_SOURCE_DIR}/proto"
         --experimental_allow_proto3_optional
         "${CMAKE_SOURCE_DIR}/proto/vm.proto"
  )

  add_library(ProtoFiles
    "${CMAKE_SOURCE_DIR}/proto/vm.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/aliasreader.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/appsender.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/keystore.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/messenger.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/sharedmemory.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/rpcdb.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/metrics.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/vm.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/aliasreader.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/appsender.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/keystore.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/messenger.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/sharedmemory.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/rpcdb.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/metrics.pb.h"
  )

  # You HAVE to set the file names
  add_library (gen-grpc
    "${CMAKE_SOURCE_DIR}/proto/vm.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/aliasreader.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/appsender.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/keystore.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/messenger.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/sharedmemory.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/rpcdb.grpc.pb.cc"
    "${CMAKE_SOURCE_DIR}/proto/vm.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/aliasreader.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/appsender.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/keystore.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/messenger.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/sharedmemory.grpc.pb.h"
    "${CMAKE_SOURCE_DIR}/proto/rpcdb.grpc.pb.h"
    ${ProtoFiles}
  )

  target_link_libraries(gen-grpc PUBLIC ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} absl::flags)

  add_library(subnetooor_lib STATIC
    ${UTILS_HEADERS}
    ${UTILS_SOURCES}
    #${CONTRACT_HEADERS}
    #${CONTRACT_SOURCES}
    ${CORE_HEADERS}
    ${CORE_SOURCES}
    ${NET_HEADERS}
    ${NET_SOURCES}
  )

  add_dependencies(subnetooor_lib gen-grpc ProtoFiles)

  target_include_directories(subnetooor_lib PUBLIC ${CMAKE_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR})

  target_link_libraries(subnetooor_lib PRIVATE
    ${CRYPTOPP_LIBRARIES} ${SCRYPT_LIBRARY} Secp256k1 Ethash ${ETHASH_BYPRODUCTS} ${Protobuf_LIBRARIES}
    ${GRPC_LIBRARIES} ${CARES_LIBRARY} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} absl::flags
    ${LEVELDB_LIBRARY} ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES}
  )

  set_target_properties(subnetooor_lib PROPERTIES COMPILE_FLAGS "-DAVALANCHEGO_COMPATIBLE=1")

  # Compile and link the executable
  add_executable(subnetooord "${CMAKE_SOURCE_DIR}/src/main.cpp")

  add_dependencies(subnetooord subnetooor_lib gen-grpc ProtoFiles)
  target_include_directories(subnetooord PRIVATE subnetooor_lib ${OPENSSL_INCLUDE_DIR})
  target_link_libraries(subnetooord
    subnetooor_lib
    ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} ${LEVELDB_LIBRARY}
    ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
    absl::flags Secp256k1
  )

  # TODO: Implement tests for AvalancheGo compilation.
else()
  add_library(subnetooor_lib STATIC
    ${UTILS_HEADERS}
    ${UTILS_SOURCES}
    #${CONTRACT_HEADERS}
    #${CONTRACT_SOURCES}
    ${CORE_HEADERS}
    ${CORE_SOURCES}
    ${NET_HEADERS}
    ${NET_SOURCES}
  )

  target_include_directories(subnetooor_lib PRIVATE ${CMAKE_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR})

  target_link_libraries(subnetooor_lib PRIVATE
    ${CRYPTOPP_LIBRARIES} ${SCRYPT_LIBRARY} Secp256k1 Ethash ${ETHASH_BYPRODUCTS}
    ${LEVELDB_LIBRARY} ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES}
  )

  set_target_properties(subnetooor_lib PROPERTIES COMPILE_FLAGS "-DAVALANCHEGO_COMPATIBLE=0")

  # Compile and link the executable
  add_executable(subnetooord "${CMAKE_SOURCE_DIR}/src/main.cpp")

  add_dependencies(subnetooord subnetooor_lib)
  target_include_directories(subnetooord PRIVATE subnetooor_lib ${OPENSSL_INCLUDE_DIR})
  target_link_libraries(subnetooord
    subnetooor_lib ${LEVELDB_LIBRARY} ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} Secp256k1
  )

  # Compile and link the test executable.
  add_executable(subnetooord-tests ${TESTS_HEADERS} ${TESTS_SOURCES})

  add_dependencies(subnetooord-tests subnetooor_lib)
  target_include_directories(subnetooord-tests PRIVATE subnetooor_lib ${OPENSSL_INCLUDE_DIR})
  target_link_libraries(subnetooord-tests
    subnetooor_lib ${LEVELDB_LIBRARY} ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} Secp256k1 catch2
  )
endif()

