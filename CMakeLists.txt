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

# TODO: avoid FindBoost deprecation message in CMake 3.30+ (cmake --help-policy CMP0167)

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
set(BUILD_BTVSERVER OFF CACHE BOOL "Build the BTV (Build the Void) websocket server")
set(BUILD_VARIABLES_TESTS ON CACHE BOOL "Build tests for SafeVar (Contract variables)")
set(USE_LINT OFF CACHE BOOL "Run linter on compile (clang-tidy)")
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
message("Building testnet: ${BUILD_TESTNET}")
message("Building benchmark tests: ${BUILD_BENCHMARK}")
message("Building SafeVar tests: ${BUILD_VARIABLES_TESTS}")
message("Using lint: ${USE_LINT}")

cable_add_buildinfo_library(PROJECT_NAME bdk)

# System package configs (built-in)
set(Boost_USE_STATIC_LIBS ON)
set(OPENSSL_USE_STATIC_LIBS ON)
set(Protobuf_USE_STATIC_LIBS ON)

# Find system packages (built-in)
find_package(Boost 1.83.0 REQUIRED COMPONENTS chrono filesystem program_options system thread nowide)
find_package(OpenSSL 1.1.1 REQUIRED)
find_package(Protobuf REQUIRED) # TODO: not used yet but will be, keep it for now
find_package(Threads)

# Find system packages (custom)
find_package(Cares REQUIRED) # TODO: not used yet but will be, keep it for now
find_package(CryptoPP 8.2.0 REQUIRED)
find_package(Ethash REQUIRED)
find_package(Evmc REQUIRED)
find_package(Evmone REQUIRED)
find_package(GRPC REQUIRED) # TODO: not used yet but will be, keep it for now
find_package(Keccak REQUIRED)
find_package(Scrypt REQUIRED)
find_package(Secp256k1 REQUIRED)
find_package(Speedb REQUIRED)
find_package(SQLiteCpp REQUIRED)

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
  ${UTILS_HEADERS} ${UTILS_SOURCES} ${CONTRACT_HEADERS} ${CONTRACT_SOURCES}
  ${CORE_HEADERS} ${CORE_SOURCES} ${NET_HEADERS} ${NET_SOURCES}
)

target_include_directories(bdk_lib PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR} ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
  ${EVMC_INCLUDE_DIR} ${EVMONE_INCLUDE_DIR} ${SPEEDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
)

target_link_libraries(bdk_lib PRIVATE
  ${EVMC_INSTRUCTIONS_LIBRARY} ${EVMC_LOADER_LIBRARY} ${EVMONE_LIBRARY}
  ${CRYPTOPP_LIBRARIES} ${SCRYPT_LIBRARY} ${SECP256K1_LIBRARY}
  ${ETHASH_LIBRARY} ${KECCAK_LIBRARY} ${SPEEDB_LIBRARY}
  ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} -l:liblz4.a SQLiteCpp
)

add_subdirectory(src/bins)

