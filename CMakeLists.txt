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

set(DEPENDS_PREFIX "")
if("${TOOLCHAIN_PREFIX}" STREQUAL "x86_64-w64-mingw32")
  set(DEPENDS_PREFIX "x86_64-w64-mingw32")
  message("Toolchain: x86_64-w64-mingw32")
  message("Loading Windows Libraries")
  include("${CMAKE_SOURCE_DIR}/cmake/WindowsLibraries.cmake")
else()  
  if("${TOOLCHAIN_PREFIX}" STREQUAL "x86_64-apple-darwin20")
    message("Toolchain: x86_64-apple-darwin20")
    set(DEPENDS_PREFIX "x86_64-apple-darwin20.5.0")
    message("Loading Darwin Libraries")
    include("${CMAKE_SOURCE_DIR}/cmake/DarwinLibraries.cmake")
  else()
    if("${TOOLCHAIN_PREFIX}" STREQUAL "arm-apple-darwin21.2.0")
      message("Toolchain: arm-apple-darwin21.2.0")
      set(DEPENDS_PREFIX "arm-apple-darwin21.2.0")
      include("${CMAKE_SOURCE_DIR}/cmake/DarwinArmLibraries.cmake")
      message("Loading Darwin Libraries")
    else() 
      message("Toolchain: x86_64-pc-linux-gnu")
      set(DEPENDS_PREFIX "x86_64-pc-linux-gnu")
      include("${CMAKE_SOURCE_DIR}/cmake/LinuxLibraries.cmake")
      message("Loading Linux Libraries")
    endif()
  endif()
endif()


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
include(cmake/ProjectBoostCertify.cmake)
include(cmake/ProjectSnappy.cmake)
include(cmake/ProjectLevelDB.cmake)
find_package(Threads)

# Include Protobuf
include(cmake/ProjectProtobuf.cmake)
include(cmake/FindGRPC.cmake)
find_package(absl CONFIG REQUIRED)

# Include headers and libs
include_directories(
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/build"
  "${CMAKE_SOURCE_DIR}/build/deps/include"
  "${CMAKE_SOURCE_DIR}/depends/${DEPENDS_PREFIX}/include"
)
link_directories(
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/build"
  "${CMAKE_SOURCE_DIR}/build/deps/lib"
  "${CMAKE_SOURCE_DIR}/depends/${DEPENDS_PREFIX}/lib"
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
# TODO: force clean generation every time
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.grpc.pb.h"
  COMMAND "${CMAKE_BINARY_DIR}/deps/bin/protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/aliasreader.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.grpc.pb.h"
  COMMAND "${CMAKE_BINARY_DIR}/deps/bin/protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/appsender.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.grpc.pb.h"
  COMMAND "${CMAKE_BINARY_DIR}/deps/bin/protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/keystore.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.grpc.pb.h"
  COMMAND "${CMAKE_BINARY_DIR}/deps/bin/protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/messenger.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/metrics.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/metrics.grpc.pb.h"
  COMMAND "${CMAKE_BINARY_DIR}/deps/bin/protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/metrics.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.grpc.pb.h"
  COMMAND "${CMAKE_BINARY_DIR}/deps/bin/protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/sharedmemory.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.grpc.pb.h"
  COMMAND "${CMAKE_BINARY_DIR}/deps/bin/protoc"
  ARGS --grpc_out="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
       "${CMAKE_CURRENT_SOURCE_DIR}/proto/rpcdb.proto"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.cc"
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/vm.grpc.pb.h"
  COMMAND "${CMAKE_BINARY_DIR}/deps/bin/protoc"
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

# Compile and link the executable
#file(GLOB SUBNETOOOR_HEADERS "src/*.h")
#file(GLOB SUBNETOOOR_SOURCES "src/*.cpp")

set(SUBNETOOOR_HEADERS
  src/main.h
  src/subnet.h
  src/json.hpp
  src/utils.h
  src/block.h
  src/grpcserver.h
  src/db.h
  src/grpcclient.h
  src/state.h
  src/chainHead.h
)

set(SUBNETOOOR_SOURCES
  src/main.cpp
  src/subnet.cpp
  src/utils.cpp
  src/block.cpp
  src/grpcserver.cpp
  src/db.cpp
  src/grpcclient.cpp
  src/state.cpp
  src/chainHead.cpp
)

message(STATUS ${SUBNETOOOR_HEADERS})
message(STATUS ${SUBNETOOOR_SOURCES})

add_executable(subnetooord ${SUBNETOOOR_HEADERS} ${SUBNETOOOR_SOURCES}
  ${PROTO_VM_SRCS} ${PROTO_VM_HDRS} ${PROTO_METRICS_SRCS} ${PROTO_METRICS_HDRS}
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
# Add include folder to subnetooor
target_include_directories(subnetooord PRIVATE aleth)
target_link_libraries(subnetooord aleth Protobuf ${GRPC_LIBRARIES}
  ${GRPC++_LIBRARY} LevelDB snappy ${BOOST_LIBS} ${OPENSSL_LIBS} ${Z_LIBS} ${LIB_INCLUDE}
)


