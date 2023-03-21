add_executable (subnetooor_build_genesis
  genesis.cpp
)

target_include_directories(subnetooor_build_genesis PRIVATE gen-grpc ProtoFiles)
add_dependencies(subnetooor_build_genesis gen-grpc ProtoFiles)
target_link_libraries(subnetooor_build_genesis PUBLIC subnetooor_utils subnetooor_core PRIVATE ${CARES_LIBRARY} ${LEVELDB_LIBRARY}
  ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
  absl::flags Secp256k1 aleth)

install(TARGETS subnetooor_build_genesis DESTINATION bin)