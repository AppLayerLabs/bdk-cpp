# Find the RocksDB library and define the following variables:
# ROCKSDB_FOUND
# ROCKSDB_INCLUDE_DIR
# ROCKSDB_LIBRARY

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

find_path(ROCKSDB_INCLUDE_DIR NAMES rocksdb/db.h)
find_library(ROCKSDB_LIBRARY NAMES librocksdb.a)

SELECT_LIBRARY_CONFIGURATIONS(RocksDB)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  RocksDB DEFAULT_MSG
  ROCKSDB_LIBRARY ROCKSDB_INCLUDE_DIR
)

mark_as_advanced(ROCKSDB_INCLUDE_DIR ROCKSDB_LIBRARY)

