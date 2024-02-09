if(BUILD_AVALANCHEGO)

else()
  # Compile and link the test executable if set to build it
  if (BUILD_TESTS)
    add_executable(orbitersdkd-tests ${TESTS_HEADERS} ${TESTS_SOURCES})
    add_dependencies(orbitersdkd-tests orbitersdk_lib)
    target_include_directories(orbitersdkd-tests PRIVATE orbitersdk_lib  ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(orbitersdkd-tests
        orbitersdk_lib Speedb ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} Secp256k1 catch2 Ethash ${ETHASH_BYPRODUCTS}
    )
  endif()
endif()