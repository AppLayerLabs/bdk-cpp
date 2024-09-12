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
    ${SPEEDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )

  target_link_libraries(network_sim_lib PRIVATE bdk_lib
    ${CRYPTOPP_LIBRARIES} ${SCRYPT_LIBRARY} ${SECP256K1_LIBRARY}
    ${ETHASH_LIBRARY} ${KECCAK_LIBRARY} ${SPEEDB_LIBRARY}
    ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES}
  )

  # Compile and link the ABI generator executable
  add_executable(network-sim "main.cpp")

  add_dependencies(network-sim bdk_lib network_sim_lib)
  
  target_include_directories(network-sim PRIVATE
    bdk_lib network_sim_lib ${OPENSSL_INCLUDE_DIR}
    ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
    ${SPEEDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )
  
  target_link_libraries(network-sim
    bdk_lib network_sim_lib
    ${SPEEDB_LIBRARY} ${SNAPPY_LIBRARY} ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} ${SECP256K1_LIBRARY}
    ${ETHASH_LIBRARY} ${KECCAK_LIBRARY}
  )
endif()

