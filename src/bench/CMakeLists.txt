file(GLOB SUBNETOOOR_BENCH_SRCS "*.cpp")
file(GLOB SUBNETOOOR_BENCH_HDRS "*.h")

add_library (subnetooor_bench
  ${SUBNETOOOR_BENCH_SRCS}
  ${SUBNETOOOR_BENCH_HDRS}
)

target_include_directories(subnetooor_bench PRIVATE "../net")
add_dependencies(subnetooor_bench gen-grpc ProtoFiles)
target_link_libraries(subnetooor_bench PUBLIC subnetooor_core PRIVATE ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES} ${CARES_LIBRARY} ${LEVELDB_LIBRARY}
  ${SNAPPY_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
  absl::flags Secp256k1 aleth)