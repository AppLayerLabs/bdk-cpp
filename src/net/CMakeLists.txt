file(GLOB SUBNETOOOR_NET_SRCS "*.cpp")
file(GLOB SUBNETOOOR_NET_HDRS "*.h")

add_library (subnetooor_net
  ${SUBNETOOOR_NET_SRCS}
  ${SUBNETOOOR_NET_HDRS}
)

target_include_directories(subnetooor_net PRIVATE gen-grpc ProtoFiles)
add_dependencies(subnetooor_net gen-grpc ProtoFiles)
target_link_libraries(subnetooor_net PUBLIC subnetooor_core PRIVATE ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} ${LEVELDB_LIBRARY}
  ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
  absl::flags Secp256k1 aleth gen-grpc ProtoFiles)