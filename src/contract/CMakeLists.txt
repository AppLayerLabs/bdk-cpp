file(GLOB SUBNETOOOR_CONTRACT_SRCS "*.cpp")
file(GLOB SUBNETOOOR_CONTRACT_HDRS "*.h")

add_library (subnetooor_contract
  ${SUBNETOOOR_CONTRACT_SRCS}
  ${SUBNETOOOR_CONTRACT_HDRS}
)

target_include_directories(subnetooor_contract PRIVATE "../net")
add_dependencies(subnetooor_contract gen-grpc ProtoFiles)
target_link_libraries(subnetooor_contract PUBLIC subnetooor_core PRIVATE ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} ${LEVELDB_LIBRARY}
  ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
  absl::flags Secp256k1 aleth)