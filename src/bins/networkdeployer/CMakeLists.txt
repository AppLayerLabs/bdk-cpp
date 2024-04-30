if(BUILD_AVALANCHEGO)

else()
  # Compile and link the ABI generator executable
  add_executable(networkdeployer "main.cpp")

  add_dependencies(networkdeployer bdk_lib)
  target_include_directories(networkdeployer PRIVATE bdk_lib ${OPENSSL_INCLUDE_DIR})
  target_link_libraries(networkdeployer
      bdk_lib Speedb ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} Secp256k1 Ethash ${ETHASH_BYPRODUCTS}
  )

endif()