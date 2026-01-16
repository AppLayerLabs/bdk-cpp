# Compile and link the Discovery Node test executable if set to build it
if (BUILD_DISCOVERY)
  add_executable(bdkd-discovery "main.cpp")

  add_dependencies(bdkd-discovery bdk_lib)

  target_include_directories(bdkd-discovery PRIVATE
    bdk_lib ${OPENSSL_INCLUDE_DIR} ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
    ${ROCKSDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )

  target_link_libraries(bdkd-discovery
    bdk_lib
    Boost::headers Boost::chrono Boost::container Boost::filesystem Boost::nowide Boost::program_options Boost::thread
    protobuf::libprotobuf protobuf::libprotoc
    RocksDB::rocksdb
    OpenSSL::SSL OpenSSL::Crypto
    libsecp256k1::secp256k1
    ethash::ethash ethash::keccak
    ${CMAKE_SOURCE_DIR}/deps/target/lib/libevmone.a
  )
endif()

