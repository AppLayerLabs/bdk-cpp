# orbitersdk

Sparq subnet source code. See [Chapter 3.3 on the docs](https://github.com/SparqNet/sparq-docs/blob/main/Sparq_en-US/ch3/3-3.md) for an expanded version of this file.

If you are a developer, fill this form out for free support and additional incentives: https://forms.gle/m83ceG3XoJY3fpwU9

## Developing on Docker

The project has a Dockerfile at the root of the repository that will build the project and deploy the network. It will also install tmux and vim for convenience. To develop on Docker, follow these steps:

* Install Docker on your machine:
  * [Docker for Windows](https://docs.docker.com/docker-for-windows/install/)
  * [Docker for Mac](https://docs.docker.com/docker-for-mac/install/)
  * [Docker for Linux](https://docs.docker.com/desktop/install/linux-install/)
* Build the image locally with `docker build -t orbitersdk-cpp-dev:latest .` (if using Linux or Mac, run as `sudo`)
  * This will build the image and tag it as `orbitersdk-cpp-dev:latest` - you can change the tag to whatever you want, but remember to change it on the next step
* Run the container (you will be logged in as root):
  * **For Linux/Mac**: `sudo docker run -it -v $(pwd):/orbitersdk-volume -p 8080-8099:8080-8099 -p 8110-8111:8110-8111 orbitersdk-cpp-dev:latest`
  * **For Windows**: `docker run -it -v %cd%:/orbitersdk-volume -p 8080-8099:8080-8099 -p 8110-8111:8110-8111 orbitersdk-cpp-dev:latest`

Remeber that we are using our local SDK repo as a volume, so every change in the local folder will be reflected to the container in real time, and vice-versa.

Also, you can integrate the container with your favorite IDE or editor, e.g. [VSCode + Docker extension](https://marketplace.visualstudio.com/items?itemName=ms-azuretools.vscode-docker).

## Developing manually

Install the following dependencies on your system:

* **GCC** with support for **C++20** or higher
* **CMake 3.19.0** or higher
* **Boost 1.74** or higher (components: *chrono, filesystem, program-options, system, thread, nowide*)
* **CryptoPP 8.2.0** or higher
* **libscrypt**
* **OpenSSL 1.1.1**
* **zlib**
* **libsnappy** for database compression
* (optional) **clang-tidy** for linting

If building with AvalancheGo support, you'll also need:

* **Abseil (absl)**
* **libc-ares**
* **Protobuf 3.12** or higher
* **gRPC**

### One-liners

For **Debian 11 Bullseye or newer**:
* `sudo apt install build-essential cmake tmux clang-tidy autoconf libtool pkg-config libabsl-dev libboost-all-dev libc-ares-dev libcrypto++-dev libgrpc-dev libgrpc++-dev librocksdb-dev libscrypt-dev libsnappy-dev libssl-dev zlib1g-dev openssl protobuf-compiler protobuf-compiler-grpc`

#### Caveats

* **Debian 11 Bullseye and older**: CMake version from repos is too old (3.18.4), has to be installed manually from [their website](https://cmake.org/download)

## Documentation

We use [Doxygen](https://www.doxygen.nl/index.html) to generate documentation over the current source code, To generate the documentation, run `doxygen` inside the project's root folder.

For a more detailed explanation of the project's structure, check the [docs](https://github.com/SparqNet/sparq-docs/tree/main/Sparq_en-US) repository.

## Compiling

* Clone the project: `git clone https://github.com/SparqNet/orbitersdk-cpp
* Go to the project's root folder, create a "build" folder and change to it:
  * `cd orbitersdk-cpp && mkdir build && cd build`
* Run `cmake` inside the build folder: `cmake ..`
  * Use `-DCMAKE_BUILD_TYPE=RelWithDebInfo` to build with debug symbols
  * Use `-DDEBUG=OFF` to build without debug symbols (ON by default)
  * Use `-DUSE_LINT=ON` to run clang-tidy along the build (this is OFF by default, WILL TAKE SIGNIFICANTLY LONGER TO COMPILE)
* Build the executable: `cmake --build . -- -j$(nproc)`
  * If using the linter, pipe the stderr output to a file, e.g. `cmake --build . -- -j$(nproc) 2> log.txt`

## Deploying

Go back to the project's root directory and run `./scripts/AIO-setup.sh`. The script will deploy a local testnet with 5 Validator nodes, 6 normal nodes and 1 discovery node. All of them will be rdPoS. Run the script again to re-deploy, or call `tmux kill-server` to stop it entirely.

You can use the following flags on the script to customize deployment:

| Flag | Description | Default Value |
|------|-------------|---------------|
| --clean | Clean the build folder before building | false |
| --no-deploy | Only build the project, don't deploy the network | false |
| --debug=\<bool\> | Build in debug mode | true |
| --cores=\<int\> | Number of cores to use for building | Maximum available |

Example: `./scripts/AIO-setup.sh --clean --no-deploy --debug=false --cores=4` will clean the build folder, only build the project, build in release mode and use 4 cores for building. Remember that GCC uses around 1.5GB of RAM per core, so, for stability, it is recommended to adjust the number of cores to the available RAM on your system.

Once deployed, you can connect any Web3 client to it (we recommend [Metamask](https://metamask.io)). See below for details.

Note that, when re-deploying, if your wallet or RPC client keeps track of account nonce data, you must reset it as a network reset would set back their nonces to 0. [Here's how to do it in MetaMask, for example](https://support.metamask.io/hc/en-us/articles/360015488891-How-to-clear-your-account-activity-reset-account).

## Testnet defaults

The deployed chain will have the following information by default:

- ChainID: 808080
- Owner: 0x00dead00665771855a34155f5e7405489df2c3c6
- Owner Private Key: 0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867
- Owner Initial Balance: 1000000000000000000000 wei
- ContractManager Address: 0x0001cb47ea6d8b55fe44fdd6b1bdb579efb43e61
- rdPoS Address: 0xb23aa52dbeda59277ab8a962c69f5971f22904cf
- Default RPC URL: http://127.0.0.1:8090

Nodes will be deployed on the same machine, under the following ports and tmux sessions:

| Session Name             | Node Type | P2P Port | HTTP Port | Validator Key                                                      |
|--------------------------|-----------|----------|-----------|--------------------------------------------------------------------|
| local_testnet_discovery  | Discovery | 8080     | 8090      | XXXXX                                                              |
| local_testnet_validator1 | Validator | 8081     | 8090      | 0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759 |
| local_testnet_validator2 | Validator | 8082     | 8091      | 0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4 |
| local_testnet_validator3 | Validator | 8083     | 8092      | 0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751 |
| local_testnet_validator4 | Validator | 8084     | 8093      | 0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7 |
| local_testnet_validator5 | Validator | 8085     | 8094      | 0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6 |
| local_testnet_normal1    | Normal    | 8086     | 8095      | XXXX                                                               |
| local_testnet_normal2    | Normal | 8087     | 8096      | XXXX |
| local_testnet_normal3    | Normal | 8088     | 8097      | XXXX |
| local_testnet_normal4    | Normal | 8089     | 8098      | XXXX |
| local_testnet_normal5    | Normal | 8110     | 8099      | XXXX |
| local_testnet_normal6    | Normal | 8111     | 8100      | XXXX |

