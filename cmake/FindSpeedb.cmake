# Find the Speedb library and define the following variables:
# SPEEDB_FOUND
# SPEEDB_INCLUDE_DIR
# SPEEDB_LIBRARY

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

find_path(SPEEDB_INCLUDE_DIR NAMES db.h PATH_SUFFIXES rocksdb)
find_library(SPEEDB_LIBRARY NAMES libspeedb.a)

SELECT_LIBRARY_CONFIGURATIONS(Speedb)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  Speedb DEFAULT_MSG
  SPEEDB_LIBRARY SPEEDB_INCLUDE_DIR
)

mark_as_advanced(SPEEDB_INCLUDE_DIR SPEEDB_LIBRARY)

