# Compile and link the ABI generator executable
add_executable(networkdeployer "main.cpp")

add_dependencies(networkdeployer bdk_lib)

target_include_directories(networkdeployer PRIVATE
  bdk_lib ${OPENSSL_INCLUDE_DIR} ${ETHASH_INCLUDE_DIR} ${KECCAK_INCLUDE_DIR}
  ${SPEEDB_INCLUDE_DIR} ${SECP256K1_INCLUDE_DIR}
)

target_link_libraries(networkdeployer
  bdk_lib ${SPEEDB_LIBRARY} ${SNAPPY_LIBRARY} ${Boost_LIBRARIES}
  ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} ${SECP256K1_LIBRARY}
  ${ETHASH_LIBRARY} ${KECCAK_LIBRARY}
)

