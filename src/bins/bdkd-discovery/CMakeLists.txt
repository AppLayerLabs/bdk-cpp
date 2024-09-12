# Compile and link the Discovery Node test executable if set to build it
if (BUILD_DISCOVERY)
  add_executable(bdkd-discovery "main.cpp")

  add_dependencies(bdkd-discovery bdk_lib)

  target_include_directories(bdkd-discovery PRIVATE
    bdk_lib ${OPENSSL_INCLUDE_DIR} ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
    ${SPEEDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
  )

  target_link_libraries(bdkd-discovery
    bdk_lib ${SPEEDB_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES} ${SECP256K1_LIBRARY} ${ETHASH_LIBRARY} ${KECCAK_LIBRARY}
  )
endif()

