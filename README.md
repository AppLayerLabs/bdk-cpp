# subnetooor

Sparq subnet source code.

## Requirements
* **GCC** with support for **C++17** or higher
* **CMake 3.19.0** or higher
* **Abseil (absl)**
* **Boost 1.74** or higher (components: *chrono, filesystem, program-options, system, thread, nowide*)
* **CryptoPP 8.2.0** or higher
* **gRPC** + **Protobuf 3.12** or higher + **libc-ares**
* **LevelDB** + **libsnappy**
* **libscrypt**
* **OpenSSL 1.1.1**
* **zlib**
* (optional) **clang-tidy** for linting

### One-liners

For **Debian 11 Bullseye or newer**:
* `sudo apt install build-essential cmake clang-tidy autoconf libtool pkg-config libabsl-dev libboost-{chrono,filesystem,program-options,system,thread,nowide}-dev libc-ares-dev libcrypto++-dev libgrpc-dev libgrpc++-dev libleveldb-dev libscrypt-dev libsnappy-dev libssl-dev zlib1g-dev openssl protobuf-compiler protobuf-compiler-grpc`

#### Caveats

* **Debian 11 Bullseye and older**: CMake version from repos is too old (3.18.4), has to be installed manually from [their website](https://cmake.org/download)

## Compiling
* Clone the project: `git clone https://github.com/subnetoors/subnetooor`
* Go to the project's root folder, create a "build" folder and change to it:
  * `cd subnetooor && mkdir build && cd build`
* Run `cmake` inside the build folder: `cmake ..`
  * Use `-DCMAKE_BUILD_TYPE=RelWithDebInfo` to build with debug symbols
  * Use `-DUSE_LINT=ON` to run clang-tidy along the build (this is off by default, WILL TAKE SIGNIFICANTLY LONGER TO COMPILE)
* Build the executable: `cmake --build . -- -j$(nproc)`
  * If using the linter, pipe the stderr output to a file, e.g. `cmake --build . -- -j$(nproc) 2> log.txt`

## Deploying

If you want a **manual** deploy, check [DEPLOY_SUBNET.md](DEPLOY_SUBNET.md).

If you want an **assisted/automatic** deploy, or you don't care about the details and just want to get it running ASAP, use one of the scripts in the `scripts` folder:

* **AIO-setup.sh** - Setup the Subnet with a single node.
* **AIO-setup-two-nodes.sh** - Setup the Subnet with two nodes.

After the Subnet is deployed, use the scripts under the `scripts/helpers` subfolder to manage it:

* **startAll.sh** - Start the Subnet.
* **killAll.sh** - Stop the Subnet.
* **cleanAll.sh** - Clean the Subnet logs.

