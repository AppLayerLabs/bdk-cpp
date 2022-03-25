cd proto
protoc --grpc_out=. --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin vm.proto
protoc --cpp_out=. vm.proto
protoc --cpp_out=. metrics.proto

g++ -c metrics.pb.cc metrics.pb.h vm.grpc.pb.cc vm.grpc.pb.h vm.pb.cc vm.pb.h -lprotobuf
cd ..
g++ src/main.cpp -o storageVM proto/metrics.pb.o proto/vm.grpc.pb.o proto/vm.pb.o `pkg-config --libs grpc++` -lprotobuf -lpthread -lgrpc++_reflection