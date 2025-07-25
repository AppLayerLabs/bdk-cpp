# Find the Ethash libraries and define the following variables:
# ETHASH_FOUND
# ETHASH_INCLUDE_DIR
# ETHASH_LIBRARY

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

find_path(ETHASH_INCLUDE_DIR NAMES ethash.h PATH_SUFFIXES ethash)
find_library(ETHASH_LIBRARY NAMES libethash.a)

SELECT_LIBRARY_CONFIGURATIONS(Ethash)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  Ethash DEFAULT_MSG
  ETHASH_LIBRARY ETHASH_INCLUDE_DIR
)

mark_as_advanced(ETHASH_INCLUDE_DIR ETHASH_LIBRARY)

