if(BUILD_AVALANCHEGO)
  # Compile and link the executable
  add_executable(orbitersdkd "main.cpp")

  add_dependencies(orbitersdkd orbitersdk_lib gen-grpc ProtoFiles)
  target_include_directories(orbitersdkd PRIVATE orbitersdk_lib ${OPENSSL_INCLUDE_DIR})
  target_link_libraries(orbitersdkd
      orbitersdk_lib
      ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} Speedb
      ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
      absl::flags Secp256k1 Ethash ${ETHASH_BYPRODUCTS}
  )
else()
  # Compile and link the executable
  add_executable(orbitersdkd "main.cpp")

  add_dependencies(orbitersdkd orbitersdk_lib)
  target_include_directories(orbitersdkd PRIVATE orbitersdk_lib ${OPENSSL_INCLUDE_DIR})
  target_link_libraries(orbitersdkd
      orbitersdk_lib Speedb ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} Secp256k1 Ethash ${ETHASH_BYPRODUCTS}
  )
endif()