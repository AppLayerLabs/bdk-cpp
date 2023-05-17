include(ExternalProject)

if (MSVC)
  set(_only_release_configuration -DCMAKE_CONFIGURATION_TYPES=Release)
  set(_overwrite_install_command INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install)
endif()

set(prefix "${CMAKE_BINARY_DIR}/deps")
set(SPEEDB_LIBRARY "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}speedb${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(SPEEDB_INCLUDE_DIR "${prefix}/include")

ExternalProject_Add(
  speedb
  PREFIX "${prefix}"
  DOWNLOAD_NAME speedb-2.4.1.tar.gz
  DOWNLOAD_NO_PROGRESS 1
  GIT_REPOSITORY https://github.com/speedb-io/speedb
  GIT_TAG "speedb/v2.4.1"
  #URL https://github.com/speedb-io/speedb/releases/download/speedb/v2.4.1/speedb-2.4.1.tar.gz
  #URL_HASH SHA256=4e984515bbed0942d4ba22d8a219c752b0679d261a4baf7ac72c206f5ab1cd04
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             -DCMAKE_POSITION_INDEPENDENT_CODE=${BUILD_SHARED_LIBS}
             -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
             -DCMAKE_BUILD_TYPE=Release
             ${_only_release_configuration}
             -DCMAKE_INSTALL_LIBDIR=lib
             -DROCKSDB_BUILD_SHARED=OFF
             -DFAIL_ON_WARNINGS=OFF
             -DWITH_GFLAGS=OFF
             -DWITH_RUNTIME_DEBUG=OFF
             -DWITH_TESTS=OFF
             -DWITH_BENCHMARK_TOOLS=OFF
             -DWITH_CORE_TOOLS=OFF
             -DWITH_TOOLS=OFF
             -DWITH_TRACE_TOOLS=OFF
  ${_overwrite_install_command}
)

# Create imported library
add_library(Speedb STATIC IMPORTED)
set_property(TARGET Speedb PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Speedb PROPERTY IMPORTED_LOCATION_RELEASE "${SPEEDB_LIBRARY}")
set_property(TARGET Speedb PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SPEEDB_INCLUDE_DIR}")
add_dependencies(Speedb speedb)
