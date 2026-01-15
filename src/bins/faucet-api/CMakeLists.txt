if (BUILD_FAUCET)
  add_library(rollup_faucet_lib STATIC
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/jsonrpc/encoding.h
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/jsonrpc/decoding.h
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/jsonrpc/encoding.cpp
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/jsonrpc/decoding.cpp
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/httplistener.h
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/httpparser.h
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/httpserver.h
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/httpsession.h
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/faucetmanager.h
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/httplistener.cpp
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/httpparser.cpp
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/httpserver.cpp
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/httpsession.cpp
    ${CMAKE_SOURCE_DIR}/src/bins/faucet-api/src/faucetmanager.cpp
  )

  target_include_directories(rollup_faucet_lib PRIVATE
    ${CMAKE_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR} bdk_lib
    ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
    ${ROCKSDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )

  target_link_libraries(rollup_faucet_lib PRIVATE bdk_lib
    Boost::headers Boost::chrono Boost::container Boost::filesystem Boost::nowide Boost::program_options Boost::thread
    ${CRYPTOPP_LIBRARIES}
    libsecp256k1::secp256k1
    ethash::ethash ethash::keccak
    RocksDB::rocksdb
    OpenSSL::SSL OpenSSL::Crypto
    bdk_lib
  )

  # Compile and link the faucet-api executable
  add_executable(faucet-api "main.cpp")

  add_dependencies(faucet-api bdk_lib rollup_faucet_lib)

  target_include_directories(faucet-api PRIVATE
    bdk_lib rollup_faucet_lib ${OPENSSL_INCLUDE_DIR}
    ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
    ${ROCKSDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )

  target_link_libraries(faucet-api
    bdk_lib rollup_faucet_lib
    Boost::headers Boost::chrono Boost::container Boost::filesystem Boost::nowide Boost::program_options Boost::thread
    RocksDB::rocksdb
    OpenSSL::SSL OpenSSL::Crypto
    libsecp256k1::secp256k1
    ethash::ethash ethash::keccak
  )

  # Compile and link the faucet-api executable
  add_executable(faucet-tester "main-tester.cpp")

  add_dependencies(faucet-tester bdk_lib rollup_faucet_lib)

  target_include_directories(faucet-tester PRIVATE
    bdk_lib rollup_faucet_lib ${OPENSSL_INCLUDE_DIR}
    ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
    ${ROCKSDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )

  target_link_libraries(faucet-tester
    bdk_lib rollup_faucet_lib
    Boost::headers Boost::chrono Boost::container Boost::filesystem Boost::nowide Boost::program_options Boost::thread
    RocksDB::rocksdb
    OpenSSL::SSL OpenSSL::Crypto
    libsecp256k1::secp256k1
    ethash::ethash ethash::keccak
  )
endif()

