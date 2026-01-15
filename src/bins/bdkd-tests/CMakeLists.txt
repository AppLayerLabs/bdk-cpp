# Compile and link the test executable if set to build it
if (BUILD_TESTS)
  add_executable(bdkd-tests ${TESTS_HEADERS} ${TESTS_SOURCES})

  add_dependencies(bdkd-tests bdk_lib)

  target_include_directories(bdkd-tests PRIVATE
    bdk_lib ${OPENSSL_INCLUDE_DIR} ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
    ${ROCKSDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )

  target_link_libraries(bdkd-tests PRIVATE
    bdk_lib catch2
    Boost::headers Boost::chrono Boost::container Boost::filesystem Boost::nowide Boost::program_options Boost::thread
    protobuf::libprotobuf protobuf::libprotoc
    RocksDB::rocksdb
    OpenSSL::SSL OpenSSL::Crypto
    libsecp256k1::secp256k1
    ethash::ethash ethash::keccak
    ${CMAKE_SOURCE_DIR}/deps/target/lib/libevmone.a
  )
endif()

