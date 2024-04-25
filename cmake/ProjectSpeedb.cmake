include(ExternalProject)

if (MSVC)
  set(_only_release_configuration -DCMAKE_CONFIGURATION_TYPES=Release)
  set(_overwrite_install_command INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install)
endif()

set(prefix "${CMAKE_BINARY_DIR}/deps")
set(SPEEDB_LIBRARY "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}speedb${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(SPEEDB_INCLUDE_DIR "${prefix}/include")

set(SPEEDB_VERSION "2.8.0")

# Top: precompiled binary, bottom: compiled from source
# (Uncomment) either depending on the necessity

# TODO: doesn't work, fails compile with -fPIE related errors, try to fix this someday
#ExternalProject_Add(
#  speedb
#  PREFIX "${prefix}"
#  URL https://github.com/speedb-io/speedb/releases/download/speedb/v${SPEEDB_VERSION}/speedb-${SPEEDB_VERSION}.tar.gz
#  URL_HASH SHA256=b1d4ce4ec4d4f30e0d525c704a2079bfc15684043faf29e907eba519a7d7f930
#  DOWNLOAD_NAME speedb-${SPEEDB_VERSION}.tar.gz
#  DOWNLOAD_NO_PROGRESS 1
#  CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E copy_directory "${prefix}/src/speedb/include/rocksdb" "${SPEEDB_INCLUDE_DIR}/rocksdb"
#  BUILD_COMMAND ""
#  INSTALL_COMMAND "${CMAKE_COMMAND}" -E copy "${prefix}/src/speedb/lib64/libspeedb.a" "${SPEEDB_LIBRARY}"
#)

ExternalProject_Add(
  speedb
  PREFIX "${prefix}"
  GIT_REPOSITORY https://github.com/speedb-io/speedb
  GIT_TAG "speedb/v${SPEEDB_VERSION}"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${prefix}
             -DCMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE}
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
  BUILD_BYPRODUCTS "${SPEEDB_LIBRARY}"
  UPDATE_COMMAND ""
)

# Create imported library
add_library(Speedb STATIC IMPORTED)
set_property(TARGET Speedb PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Speedb PROPERTY IMPORTED_LOCATION_RELEASE "${SPEEDB_LIBRARY}")
set_property(TARGET Speedb PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SPEEDB_INCLUDE_DIR}")
add_dependencies(Speedb speedb SPEEDB_LIBRARY)

