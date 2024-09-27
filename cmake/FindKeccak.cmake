# Find the Keccak libraries and define the following variables:
# KECCAK_FOUND
# KECCAK_INCLUDE_DIR
# KECCAK_LIBRARY

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

find_path(KECCAK_INCLUDE_DIR NAMES keccak.h PATH_SUFFIXES ethash)
find_library(KECCAK_LIBRARY NAMES libkeccak.a)

SELECT_LIBRARY_CONFIGURATIONS(Keccak)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  Keccak DEFAULT_MSG
  KECCAK_LIBRARY KECCAK_INCLUDE_DIR
)

mark_as_advanced(KECCAK_INCLUDE_DIR KECCAK_LIBRARY)

