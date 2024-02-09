if(BUILD_AVALANCHEGO)

else()
  # Compile and link the Discovery Node test executable if set to build it
  if (BUILD_DISCOVERY)
    add_executable(orbitersdkd-discovery "main.cpp")
    add_dependencies(orbitersdkd-discovery orbitersdk_lib)
    target_include_directories(orbitersdkd-discovery PRIVATE orbitersdk_lib ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(orbitersdkd-discovery
        orbitersdk_lib Speedb ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} Secp256k1 Ethash ${ETHASH_BYPRODUCTS}
    )
  endif()
endif()