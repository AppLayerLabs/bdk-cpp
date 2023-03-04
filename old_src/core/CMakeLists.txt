file(GLOB SUBNETOOOR_CORE_SRCS "*.cpp")
file(GLOB SUBNETOOOR_CORE_HDRS "*.h")

add_library (subnetooor_core
  ${SUBNETOOOR_CORE_SRCS}
  ${SUBNETOOOR_CORE_HDRS}
)

target_include_directories(subnetooor_core PRIVATE "../net" gen-grpc ProtoFiles)
add_dependencies(subnetooor_core gen-grpc ProtoFiles)
target_link_libraries(subnetooor_core PUBLIC subnetooor_utils subnetooor_contract subnetooor_net PRIVATE ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} ${LEVELDB_LIBRARY}
  ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
  absl::flags Secp256k1 aleth)