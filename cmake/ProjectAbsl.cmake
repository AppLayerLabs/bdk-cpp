include(ExternalProject)

if (MSVC)
  set(_only_release_configuration -DCMAKE_CONFIGURATION_TYPES=Release)
  set(_overwrite_install_command INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install)
endif()

set(prefix "${CMAKE_BINARY_DIR}/deps")
set(ABSL_LIBRARY "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(ABSL_INCLUDE_DIR "${prefix}/include")

ExternalProject_Add(
  Absl
  PREFIX "${prefix}"
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             -DCMAKE_POSITION_INDEPENDENT_CODE=${BUILD_SHARED_LIBS}
             -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
             -DCMAKE_INSTALL_LIBDIR=lib
             -DBUILD_TESTING=OFF
             -DABSL_BUILD_TESTING=OFF
             ${_only_release_configuration}
  #BUILD_COMMAND ""
  ${_overwrite_install_command}
  LOG_CONFIGURE 1
  LOG_INSTALL 1
)

# Create imported library
add_library(absl STATIC IMPORTED)
file(MAKE_DIRECTORY "${ABSL_INCLUDE_DIR}")  # Must exist.
set_property(TARGET absl PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET absl PROPERTY IMPORTED_LOCATION_RELEASE "${ABSL_LIBRARY}")
set_property(TARGET absl PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${ABSL_INCLUDE_DIR}")
add_dependencies(absl Absl)

