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

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24+
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

# Project data
project(bdk VERSION 0.2.0 DESCRIPTION "AppLayer Blockchain Development Kit")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(DEBUG ON CACHE BOOL "Debug mode")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a") # Always look for static libraries - "ZLIB_USE_STATIC_LIBS" was added in 3.24
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy

# Set compiler flags
# TODO: -Wno-c++26-extensions is included because zpp_libs uses name-independent declarations (vars named "_").
# This should be resolved at a later date, but was turned off for now because it doesn't affect us
if(DEBUG)
  set(CMAKE_CXX_FLAGS "-O0 -g -Wno-c++26-extensions -fsanitize=address -fno-inline -fno-eliminate-unused-debug-types -fstack-protector") # Provides faster compile time.
elseif(SONARQUBE_ANALYSIS)
  set(CMAKE_CXX_FLAGS "-O0 -g -Wno-c++26-extensions --coverage")
else()
  set(CMAKE_CXX_FLAGS "-O2 -Wno-c++26-extensions -Werror=unused-variable")
endif()
find_program(MOLD "mold") # Use mold by default if it is installed
if(MOLD)
  message(STATUS "Using mold as linker")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=mold")
endif()

# Set project version inside the code (forcefully so changes in the .in file are always reflected correctly to the compiler)
# if (EXISTS ${CMAKE_SOURCE_DIR}/src/utils/options.h)
#   file(REMOVE ${CMAKE_SOURCE_DIR}/src/utils/options.h)
# endif()
configure_file(
  ${CMAKE_SOURCE_DIR}/src/utils/options.h.in
  ${CMAKE_SOURCE_DIR}/src/utils/options.h
  @ONLY
)

# External project data
set(BUILD_TESTS ON CACHE BOOL "Build helper unit testing program")
set(BUILD_DISCOVERY ON CACHE BOOL "Build helper discovery node program")
set(BUILD_TOOLS OFF CACHE BOOL "Build tools related to subnet")
set(BUILD_TESTNET OFF CACHE BOOL "Build the project for testnet")
set(BUILD_BENCHMARK OFF CACHE BOOL "Build with the benchmark tests")
set(USE_LINT OFF CACHE BOOL "Run linter on compile (clang-tidy)")
set(BUILD_VARIABLES_TESTS ON CACHE BOOL "Build tests for SafeVar (Contract variables)")
if(USE_LINT)
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.;-checks=-*,abseil-*,boost-*,bugprone-*,cert-*,clang-analyzer-*,concurrency-*,cppcoreguidelines-*,hicpp-*,misc-*,modernize-*,performance-*,portability-*,readability-*")
endif()

if(BUILD_TESTNET)
  add_definitions(-DBUILD_TESTNET)
endif()

# Echo CMake vars during config
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ standard is required: ${CMAKE_CXX_STANDARD_REQUIRED}")
message(STATUS "C++ extensions: ${CMAKE_CXX_EXTENSIONS}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Using PIC: ${CMAKE_POSITION_INDEPENDENT_CODE}")
message(STATUS "Find libs with suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")
message("Building tests: ${BUILD_TESTS}")
message("Building Discovery Node: ${BUILD_DISCOVERY}")
message("Building tools: ${BUILD_TOOLS}")
message("Using lint: ${USE_LINT}")

cable_add_buildinfo_library(PROJECT_NAME bdk)

# System package configs (built-in)
set(Boost_USE_STATIC_LIBS ON)
set(OPENSSL_USE_STATIC_LIBS ON)
set(Protobuf_USE_STATIC_LIBS ON)





#  ************ CUSTOM GRPC / PROTO *********************************************************

# IMPORTANT!
#
# For compilation to work in this branch, you MUST follow these steps:
#
#    ----->    https://grpc.io/docs/languages/cpp/quickstart/    <----
#
# ... to download, compile and install the latest GRPC, Protobuf, and all their dependencies
#     in a local directory in your machine (I recommend using ~/.install instead of ~/.local).
#
# This local GRPC installation instructions will get ALL the dependencies of the latest GRPC
#     version as submodules and compile them as well. Then it is just a matter of having our
#     CMake pipeline pick up on it and link against the correct libraries that are referenced
#     by the correct CMake variables (this can be done using the CMake scripts from the GRPC
#     cpp examples repository).
#
# TODO: This should be done by scripts/deps.sh instead, and then through changing the
#       PATH variable (or some other mechanism) the cmake/FindGRPCLocal.cmake script
#       below should be able to find it (that script can be modified if needed).

# This finds Protobuf and gRPC that have been built locally under
#   e.g. ~/.local or ~/.install and then make install'd
#
# NOTE: This works if and when your PATH variable points to e.g. ~/.install/bin
#       (or ~/.local/bin or whatever local directory you built gRPC in)
#       Yes, CMake uses the PATH variable to find the gRPC, Protobuf, etc. files that it needs
#       You can use:
#           cmake .. --debug-find
#       To see this in action (try removing the bin directory from the PATH)
#
include(cmake/FindGRPCLocal.cmake)

# Here we will do the code generation of all the cometbft proto files (with the has_vote hack)
#
# Using the old protoc logic which did work, will generate target gen-proto-grpc
#
include(cmake/JustGenerateCometBFTProtoV1.cmake)

#  ************ CUSTOM GRPC / PROTO *********************************************************





# Find system packages (built-in)
find_package(Threads)
find_package(Boost 1.83.0 REQUIRED COMPONENTS chrono filesystem program_options system thread nowide)
#  ************ CUSTOM GRPC / PROTO *********************************************************
# GRPC comes with libcrypto already
# Need to comment this out otherwise you get duplicate symbol definition when you link against gen-proto-grpc.
#find_package(OpenSSL 1.1.1 REQUIRED)
#  ************ CUSTOM GRPC / PROTO *********************************************************
find_package(ZLIB REQUIRED)

# Find system packages (custom)
find_package(CryptoPP 8.2.0 REQUIRED)
find_package(Scrypt REQUIRED)

# Add external modules
include(cmake/ProjectEthash.cmake)        # Ethash
include(cmake/ProjectSecp256k1.cmake)     # Bitcoin core fast implementation
include(cmake/ProjectSpeedb.cmake)        # Speedb (Level/RocksDB drop-in replacement)
include(cmake/ProjectEVMOne.cmake)        # EVMOne (EVMOne + EVMC)


# Add catch2 as a library
add_library(catch2
  ${CMAKE_SOURCE_DIR}/src/libs/catch2/catch_amalgamated.hpp
  ${CMAKE_SOURCE_DIR}/src/libs/catch2/catch_amalgamated.cpp
)
target_include_directories(catch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/catch2)
target_compile_definitions(catch2 PRIVATE CATCH_AMALGAMATED_CUSTOM_MAIN)

# Check compiler variable sizes
include(cmake/CheckSizes.cmake)

# Include directories for headers and libs
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

# Organize, compile and link bdk libs
add_subdirectory(src/contract)
add_subdirectory(src/core)
add_subdirectory(src/net)
add_subdirectory(src/utils)
add_subdirectory(tests)

add_library(bdk_lib STATIC
  ${UTILS_HEADERS}
  ${UTILS_SOURCES}
  ${CONTRACT_HEADERS}
  ${CONTRACT_SOURCES}
  ${CORE_HEADERS}
  ${CORE_SOURCES}
  ${NET_HEADERS}
  ${NET_SOURCES}
)

# I think it works even without this line
add_dependencies(bdk_lib gen-proto-grpc)

target_include_directories(bdk_lib PRIVATE ${CMAKE_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR})

target_link_libraries(bdk_lib PRIVATE EvmcInstructions EvmcLoader EvmcTooling Evmone
  ${CRYPTOPP_LIBRARIES} ${SCRYPT_LIBRARY} Secp256k1 Ethash ${ETHASH_BYPRODUCTS}
  Speedb ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES}

  # FIXME
  # Protobuf/GRPC stack
  #${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} absl::flags
  #
  # Using the custom GRPC build directory in e.g. ~/.install and the CMake scripts
  #   we are using here makes this unnecessary, i.e. it is enough to link against
  #   gen-proto-grpc and it then just works, because gen-proto-grpc is picking
  #   up not only the object files from the cc/h generated code from the proto files,
  #   but it actually comes with GRPC and Protobuf and all its dependencies in it as well.

  # FIXME
  # The entire CometBFT v1 protocol with all GRPC interfaces (PUBLIC linkage)
  #  (depends on the Protobuf/GRPC stack linked above)
  #PUBLIC gen-grpc ProtoFiles
  PUBLIC gen-proto-grpc   # just one library with all the protos (gprc + protobuf)
                          # NEW: This also comes with the GRPC, Protobuf & their deps now
                          # If needed or wanted they can be taken out of the gen-proto-grpc target
                          #   and placed here as well (I did it that way because that's what is in
                          #   the grpc cpp examples)
)

add_subdirectory(src/bins)

