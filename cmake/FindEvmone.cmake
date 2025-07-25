# Find the EVMOne library and define the following variables:
# EVMONE_FOUND
# EVMONE_INCLUDE_DIR
# EVMONE_LIBRARY

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

find_path(EVMONE_INCLUDE_DIR NAMES evmone.h PATH_SUFFIXES evmone)
find_library(EVMONE_LIBRARY NAMES libevmone.a)

SELECT_LIBRARY_CONFIGURATIONS(Evmone)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  Evmone DEFAULT_MSG
  EVMONE_LIBRARY EVMONE_INCLUDE_DIR
)

mark_as_advanced(EVMONE_INCLUDE_DIR EVMONE_LIBRARY)

