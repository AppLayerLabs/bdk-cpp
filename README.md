# subnetooor

## Requirements
* **C++17** or higher
* **CMake 3.19** or higher
* **Protobuf 3.12** or higher
* **gRPC**
* **absl**

APT one-liner (Debian 11 or higher):
* `sudo apt install build-essential autoconf libtool pkg-config protobuf-compiler protobuf-compiler-grpc libgrpc++-dev libabsl-dev`

## Building
* `mkdir build && cd build`
* `cmake ..`
* `cmake --build . -- -j$(nproc)`

