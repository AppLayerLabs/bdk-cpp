include(ExternalProject)

if (MSVC)
    set(_only_release_configuration -DCMAKE_CONFIGURATION_TYPES=Release)
    set(_overwrite_install_command INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install)
endif()

set(prefix "${CMAKE_BINARY_DIR}/deps")
set(EVMONE_LIBRARY "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}evmone${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(EVMONE_INCLUDE_DIR "${prefix}/include")
set(EVMONE_VERSION "0.11.0")
set(EVMC_INCLUDE_DIR "${prefix}/include")
set(EVMC_INSTRUCTIONS_LIBRARY "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}evmc-instructions${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(EVMC_LOADER_LIBRARY "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}evmc-loader${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(EVMC_TOOLING_LIBRARY "${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tooling${CMAKE_STATIC_LIBRARY_SUFFIX}")

ExternalProject_Add(
        evmone
        PREFIX "${prefix}"
        GIT_REPOSITORY https://github.com/chfast/evmone
        GIT_TAG "v${EVMONE_VERSION}"
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${prefix}
        -DCMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DBUILD_SHARED_LIBS=OFF
        -DEVMC_INSTALL=ON
        -DCMAKE_INSTALL_LIBDIR=lib
        ${_only_release_configuration}
        LOG_CONFIGURE 1
        ${_overwrite_install_command}
        LOG_INSTALL 1
        BUILD_BYPRODUCTS ${EVMONE_LIBRARY}
        BUILD_BYPRODUCTS ${EVMC_INSTRUCTIONS_LIBRARY}
        BUILD_BYPRODUCTS ${EVMC_LOADER_LIBRARY}
        BUILD_BYPRODUCTS ${EVMC_TOOLING_LIBRARY}
)

# Create imported library
add_library(Evmone STATIC IMPORTED)
file(MAKE_DIRECTORY "${EVMONE_INCLUDE_DIR}")  # Must exist.
set_property(TARGET Evmone PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Evmone PROPERTY IMPORTED_LOCATION_RELEASE "${EVMONE_LIBRARY}")
set_property(TARGET Evmone PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${EVMONE_INCLUDE_DIR}")
add_dependencies(Evmone evmone ${EVMONE_LIBRARY} ${EVMONE_BYPRODUCTS})

# Create an imported target for each library
add_library(EvmcInstructions STATIC IMPORTED)
set_property(TARGET EvmcInstructions PROPERTY IMPORTED_LOCATION "${EVMC_INSTRUCTIONS_LIBRARY}")
add_dependencies(EvmcInstructions evmone)

add_library(EvmcLoader STATIC IMPORTED)
set_property(TARGET EvmcLoader PROPERTY IMPORTED_LOCATION "${EVMC_LOADER_LIBRARY}")
add_dependencies(EvmcLoader evmone)

add_library(EvmcTooling STATIC IMPORTED)
set_property(TARGET EvmcTooling PROPERTY IMPORTED_LOCATION "${EVMC_TOOLING_LIBRARY}")
add_dependencies(EvmcTooling evmone)