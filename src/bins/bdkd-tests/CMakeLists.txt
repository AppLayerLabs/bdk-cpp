# Compile and link the test executable if set to build it
if (BUILD_TESTS)
  add_executable(bdkd-tests ${TESTS_HEADERS} ${TESTS_SOURCES})

  add_dependencies(bdkd-tests bdk_lib)

  target_include_directories(bdkd-tests PRIVATE
    bdk_lib ${OPENSSL_INCLUDE_DIR} ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
    ${SPEEDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )

  target_link_libraries(bdkd-tests PRIVATE
    bdk_lib catch2
    ${SPEEDB_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
    ${SECP256K1_LIBRARY} ${ETHASH_LIBRARY} ${KECCAK_LIBRARY}
  )
endif()

