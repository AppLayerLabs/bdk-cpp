if(BUILD_AVALANCHEGO)

else()
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

    target_include_directories(network_sim_lib PRIVATE ${CMAKE_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR})

    target_link_libraries(network_sim_lib PRIVATE orbitersdk_lib
        ${CRYPTOPP_LIBRARIES} ${SCRYPT_LIBRARY} Secp256k1 Ethash ${ETHASH_BYPRODUCTS}
        Speedb ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES}
    )

      # Compile and link the ABI generator executable
    add_executable(network-sim "main.cpp")

    add_dependencies(network-sim orbitersdk_lib network_sim_lib)
    target_include_directories(network-sim PRIVATE orbitersdk_lib network_sim_lib ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(network-sim
        orbitersdk_lib network_sim_lib Speedb ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} Secp256k1 Ethash ${ETHASH_BYPRODUCTS}
    )
  endif()
endif()