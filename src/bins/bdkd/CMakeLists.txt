# Compile and link the executable
add_executable(bdkd "main.cpp")

add_dependencies(bdkd bdk_lib)

target_include_directories(bdkd PRIVATE
  bdk_lib ${OPENSSL_INCLUDE_DIR} ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
  ${BZIP2_INCLUDE_DIRS} ${SNAPPY_INCLUDE_DIR} ${ROCKSDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
)

target_link_libraries(bdkd
  bdk_lib ${BZIP2_LIBRARIES} ${SNAPPY_LIBRARY} ${ROCKSDB_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES}
  ${ZLIB_LIBRARIES} ${SECP256K1_LIBRARY} ${ETHASH_LIBRARY} ${KECCAK_LIBRARY}
)

