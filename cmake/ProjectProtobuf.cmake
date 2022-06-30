include(ExternalProject)

if (MSVC)
    set(_only_release_configuration -DCMAKE_CONFIGURATION_TYPES=Release)
    set(_overwrite_install_command INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install)
endif()

set(prefix "${CMAKE_BINARY_DIR}/deps")
set(PROTOBUF_LIBRARY "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}protobuf${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(PROTOBUF_INCLUDE_DIR "${prefix}/include")

ExternalProject_Add(
  protobuf
  PREFIX "${prefix}"
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf
  GIT_TAG "3.12.x"
  SOURCE_SUBDIR cmake/
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             -DCMAKE_POSITION_INDEPENDENT_CODE=${BUILD_SHARED_LIBS}
             -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
             -DZLIB_INCLUDE_DIR=${LIB_INCLUDE}
             -DZLIB_LIB=${ZLIB_LIBRARIES}
             -Dprotobuf_BUILD_TESTS=OFF
             -Dprotobuf_BUILD_BENCHMARKS=OFF
             ${_only_release_configuration}
             -DCMAKE_INSTALL_LIBDIR=lib
  LOG_CONFIGURE 1
  BUILD_COMMAND ""
  ${_overwrite_install_command}
  LOG_INSTALL 1
  BUILD_BYPRODUCTS "${PROTOBUF_BYPRODUCTS}"
  DEPENDS
)

# Create imported library
add_library(Protobuf STATIC IMPORTED)
file(MAKE_DIRECTORY "${PROTOBUF_INCLUDE_DIR}")  # Must exist.
set_property(TARGET Protobuf PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Protobuf PROPERTY IMPORTED_LOCATION_RELEASE "${PROTOBUF_LIBRARY}")
set_property(TARGET Protobuf PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PROTOBUF_INCLUDE_DIR}")
add_dependencies(Protobuf protobuf ${PROTOBUF_BYPRODUCTS})
