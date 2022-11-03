file(GLOB SUBNETOOOR_UTILS_SRCS "*.cpp")
file(GLOB SUBNETOOOR_UTILS_HDRS "*.h")

add_library (subnetooor_utils
  ${SUBNETOOOR_UTILS_SRCS}
  ${SUBNETOOOR_UTILS_HDRS}
)

add_dependencies(subnetooor_utils gen-grpc ProtoFiles)
target_link_libraries(subnetooor_utils PRIVATE ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} ${LEVELDB_LIBRARY}
  ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
  absl::flags Secp256k1 aleth)