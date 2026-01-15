# Compile and link the ABI generator executable
add_executable(networkdeployer "main.cpp")

add_dependencies(networkdeployer bdk_lib)

target_include_directories(networkdeployer PRIVATE
  bdk_lib ${OPENSSL_INCLUDE_DIR} ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
  ${ROCKSDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
)

target_link_libraries(networkdeployer
  bdk_lib
  Boost::headers Boost::chrono Boost::container Boost::filesystem Boost::nowide Boost::program_options Boost::thread
  RocksDB::rocksdb
  OpenSSL::SSL OpenSSL::Crypto
  libsecp256k1::secp256k1
  ethash::ethash ethash::keccak
)

