if (BUILD_NETWORK_SIM)
  add_library(network_sim_lib STATIC
    ${CMAKE_SOURCE_DIR}/src/bins/network-sim/src/common.h
    ${CMAKE_SOURCE_DIR}/src/bins/network-sim/src/httpclient.h
    ${CMAKE_SOURCE_DIR}/src/bins/network-sim/src/networksimulator.h
    ${CMAKE_SOURCE_DIR}/src/bins/network-sim/src/simulatorworker.h
    ${CMAKE_SOURCE_DIR}/src/bins/network-sim/src/common.cpp
    ${CMAKE_SOURCE_DIR}/src/bins/network-sim/src/httpclient.cpp
    ${CMAKE_SOURCE_DIR}/src/bins/network-sim/src/networksimulator.cpp
    ${CMAKE_SOURCE_DIR}/src/bins/network-sim/src/simulatorworker.cpp
  )

  target_include_directories(network_sim_lib PRIVATE
    ${CMAKE_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR}
    ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
    ${ROCKSDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )

  target_link_libraries(network_sim_lib PRIVATE bdk_lib
    Boost::headers Boost::chrono Boost::container Boost::filesystem Boost::nowide Boost::program_options Boost::thread
    ${CRYPTOPP_LIBRARIES}
    libsecp256k1::secp256k1
    ethash::ethash ethash::keccak
    RocksDB::rocksdb
    OpenSSL::SSL OpenSSL::Crypto
  )

  # Compile and link the ABI generator executable
  add_executable(network-sim "main.cpp")

  add_dependencies(network-sim bdk_lib network_sim_lib)
  
  target_include_directories(network-sim PRIVATE
    bdk_lib network_sim_lib ${OPENSSL_INCLUDE_DIR}
    ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
    ${ROCKSDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )
  
  target_link_libraries(network-sim
    bdk_lib network_sim_lib
    Boost::headers Boost::chrono Boost::container Boost::filesystem Boost::nowide Boost::program_options Boost::thread
    RocksDB::rocksdb
    OpenSSL::SSL OpenSSL::Crypto
    libsecp256k1::secp256k1
    ethash::ethash ethash::keccak
  )
endif()

