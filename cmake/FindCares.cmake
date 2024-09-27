# Find the c-ares library and define the following variables:
# CARES_FOUND
# CARES_INCLUDE_DIR
# CARES_LIBRARY

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

find_path(CARES_INCLUDE_DIR NAMES ares.h)
find_library(CARES_LIBRARY NAMES libcares_static.a libcares.a)

SELECT_LIBRARY_CONFIGURATIONS(Cares)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  Cares DEFAULT_MSG
  CARES_LIBRARY CARES_INCLUDE_DIR
)

mark_as_advanced(CARES_INCLUDE_DIR CARES_LIBRARY)

