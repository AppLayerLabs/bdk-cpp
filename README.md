# Blockchain Development Kit (BDK)

</p>
<p align="center">
    <a href="https://github.com/AppLayer/bdk-cpp/graphs/contributors" alt="Contributors">
        <img src="https://img.shields.io/github/contributors/AppLayer/bdk-cpp" /></a>
    <a href="https://github.com/AppLayer/bdk-cpp/pulse" alt="Activity">
        <img src="https://img.shields.io/github/commit-activity/m/AppLayer/bdk-cpp" /></a>
    <a href="https://github.com/AppLayer/bdk-cpp/actions/workflows/c-cpp.yml/badge.svg?branch=main">
        <img src="https://github.com/AppLayer/bdk-cpp/actions/workflows/c-cpp.yml/badge.svg?branch=main" alt="build status"></a>
    <a href="https://discord.com/channels/1072297918897340506/1085807995595788378">
        <img src="https://img.shields.io/discord/308323056592486420?logo=discord"
            alt="chat on Discord"></a>
    <a href="https://twitter.com/intent/follow?screen_name=SparqNet">
        <img src="https://img.shields.io/twitter/follow/SparqNet?style=social&logo=twitter"
            alt="follow on Twitter"></a>
  <!-- link for telegram -->
    <a href="https://t.me/SparqOfficial">
        <img src="https://img.shields.io/badge/chat-on%20telegram-blue.svg"
            alt="chat on Telegram"></a>
</p>

AppLayer's BDK source code. [See the docs](https://docs.applayer.com) for a more thorough look at the project.

If you are a developer, fill this form out for free support and additional incentives: https://forms.gle/m83ceG3XoJY3fpwU9

## Developing on Docker

The project has a Dockerfile at the root of the repository that will build the project and deploy the network. It will also install tmux and vim for convenience. To develop on Docker, follow these steps:

* Install Docker on your machine:
  * [Docker for Windows](https://docs.docker.com/docker-for-windows/install/)
  * [Docker for Mac](https://docs.docker.com/docker-for-mac/install/)
  * [Docker for Linux](https://docs.docker.com/desktop/install/linux-install/)
* Build the image locally with `docker build -t bdk-cpp-dev:latest .`
  * This will build the image and tag it as `bdk-cpp-dev:latest` - you can change the tag to whatever you want, but remember to change it on the next step
* Run the container (you will be logged in as root):
  * **For Linux/Mac**: `docker run -it --name bdk-cpp -v $(pwd):/bdk-volume -p 8080-8099:8080-8099 -p 8110-8111:8110-8111 bdk-cpp-dev:latest`
  * **For Windows**: `docker run -it --name bdk-cpp -v %cd%:/bdk-volume -p 8080-8099:8080-8099 -p 8110-8111:8110-8111 bdk-cpp-dev:latest`

Remember that we are using our local repo as a volume, so every change in the local folder will be reflected to the container in real time, and vice-versa.

Also, you can integrate the container with your favorite IDE or editor, e.g. [VSCode + Docker extension](https://marketplace.visualstudio.com/items?itemName=ms-azuretools.vscode-docker).

## Developing manually

You will need the following dependencies installed locally on your system:

* *Toolchain binaries*:
  * **git**
  * **GCC** with support for **C++23** or higher
  * **Make**
  * **CMake 3.19.0** or higher
  * **Protobuf** (protoc + grpc_cpp_plugin)
  * **tmux** (for deploying)
  * (optional) **ninja** if you prefer it over make
  * (optional) **mold** if you prefer it over ld
  * (optional) **doxygen** for generating docs
  * (optional) **clang-tidy** for linting
* *Libraries*:
  * **Boost 1.83** or higher (components: *chrono, filesystem, program-options, system, thread, nowide*)
  * **OpenSSL 1.1.1** / **libssl 1.1.1** or higher
  * **libzstd**
  * **CryptoPP 8.2.0** or higher
  * **libscrypt**
  * **libc-ares**
  * **gRPC** (libgrpc and libgrpc++)
  * **secp256k1**
  * **ethash** + **keccak**
  * **EVMOne** + **EVMC**
  * **Speedb**

The versions of those dependencies should suffice out-of-the-box for at least the following distros (or greater, including their derivatives):

* **Debian 13 (Trixie)**
* **Ubuntu 24.04 LTS (Noble Numbat)**
* **Linux Mint 22 (Wilma)**
* **Fedora 40**
* Any rolling release distro from around **May 2024** onwards (check their repos to be sure)

### Tips for dependencies

There is a script called `scripts/deps.sh` which you can use to check if you have those dependencies installed (`deps.sh --check`), install them in case you don't (`deps.sh --install`), and clean up the external ones for reinstalling (`deps.sh --cleanext`). The script expects dependencies to be installed either on `/usr` or `/usr/local`, giving preference to the latter if it finds anything there (so you can use a higher version of a dependency while still keeping your distro's default one).

**Please note that installing most dependencies through the script only works on APT-based distros** (Debian, Ubuntu and derivatives) - you can still check the dependencies on any distro and install the few ones labeled as "external" (those are fetched through `git`), but if you're on a distro with another package manager and/or a distro older than one of the minimum ones listed above, you're on your own.

For Debian specifically, you can (and should) use `update-alternatives` to register and set your GCC version to a more up-to-date build if required.

If you're using a self-compiled GCC build out of the system path (e.g. `--prefix=/usr/local/gcc-X.Y.Z` instead of `--prefix=/usr/local`), don't forget to export its installation paths in your `PATH` and `LD_LIBRARY_PATH` env vars (to prevent e.g. "version `GLIBCXX_...'/`CXXABI_...` not found" errors). Put something like this in your `~/.bashrc` file for example, changing the version accordingly to whichever one you have installed:

```bash
# For GCC in /usr/local
export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH

# For self-contained GCC outside /usr/local
export PATH=/usr/local/gcc-14.2.0/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/gcc-14.2.0/lib64:$LD_LIBRARY_PATH
```

## Documentation

We use [Doxygen](https://www.doxygen.nl/index.html) to generate documentation over the current source code. Run `doxygen` inside the project's root folder. Docs should be inside `docs/html`.

You should do this after running `cmake ..` in the build directory, as some header files need to be generated first.

For a more detailed explanation of the project's structure, check the [docs](https://github.com/AppLayer/sparq-docs/tree/main/Sparq_en-US) repository.

## Compiling

* Clone the project: `git clone https://github.com/AppLayer/bdk-cpp
* Go to the project's root folder, create a "build" folder and change to it:
  * `cd bdk-cpp && mkdir build && cd build`
* Run `cmake` inside the build folder: `cmake ..`
  * Use `-DCMAKE_BUILD_TYPE={Debug,RelWithDebInfo,Release}` to set the respective debug/release builds (Debug by default)
  * Use `-DDEBUG=OFF` to build without debug flags (ON by default)
  * Use `-DUSE_LINT=ON` to run clang-tidy along the build (OFF by default, WILL TAKE SIGNIFICANTLY LONGER TO COMPILE)
* Build the executable: `cmake --build . -- -j$(nproc)` (adjust `-j$(nproc)` accordingly if needed)
  * If using the linter, pipe stderr to a file (e.g. `cmake --build . -- -j$(nproc) 2> log.txt`)
* If you need to clean up the build, use `cmake --build . --target clean`
  * This also cleans stale `.pb.*` files in the `proto` folder (useful for if/when Protobuf gets updated)

## Configuring

Before deploying, go to the `nodeopts` directory in the project's root folder - you'll find a `defaults.json` file which is the template for a node's configuration. The template has sane default values, but if you wish, you can copy it to an `options.json` file and customize it to your liking. The deploy script (see below) will use your custom config over the default one if it exists in the folder.

NOTE: if you're deploying nodes manually for some reason (as in NOT using the deploy script), keep in mind you'll have to create and configure one `options.json` file for EACH different node you want to run. If that's the case, make sure that things like HTTP ports and the like are not conflicting with each other.

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

* ChainID: **808080**
* Chain Owner: **0x00dead00665771855a34155f5e7405489df2c3c6**
* Chain Owner Private Key: **0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867**
* Chain Owner Initial Balance: **0 wei** (this is for security purposes, if you want an initial balance please create your own `options.json` file as stated above and edit the "chainOwnerInitialBalance" to your desired value in wei)
* Genesis Private Key: **0x4d48bdf34d65ef2bed2e4ee9020a7d3162b494ac31d3088153425f286f3d3c8c**
* Genesis Address: **0x00dead001ae76ac3d3f282ffb8481becdee17357**
* Genesis Timestamp: **1726597594000000**
* ContractManager Address: **0x0001cb47ea6d8b55fe44fdd6b1bdb579efb43e61**
* rdPoS Address: **0xb23aa52dbeda59277ab8a962c69f5971f22904cf**
* Default RPC URL: **http://127.0.0.1:8090**

Nodes are all deployed on the same machine, under the following ports and tmux sessions:

| Session Name             | Node Type | P2P Port | HTTP Port | Validator Key                                                      |
|--------------------------|-----------|----------|-----------|--------------------------------------------------------------------|
| local_testnet_discovery  | Discovery | 8080     | 8090      | XXXX                                                               |
| local_testnet_validator1 | Validator | 8081     | 8090      | 0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759 |
| local_testnet_validator2 | Validator | 8082     | 8091      | 0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4 |
| local_testnet_validator3 | Validator | 8083     | 8092      | 0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751 |
| local_testnet_validator4 | Validator | 8084     | 8093      | 0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7 |
| local_testnet_validator5 | Validator | 8085     | 8094      | 0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6 |
| local_testnet_normal1    | Normal    | 8086     | 8095      | XXXX                                                               |
| local_testnet_normal2    | Normal    | 8087     | 8096      | XXXX                                                               |
| local_testnet_normal3    | Normal    | 8088     | 8097      | XXXX                                                               |
| local_testnet_normal4    | Normal    | 8089     | 8098      | XXXX                                                               |
| local_testnet_normal5    | Normal    | 8110     | 8099      | XXXX                                                               |
| local_testnet_normal6    | Normal    | 8111     | 8100      | XXXX                                                               |

