#!/bin/bash -e

# ===========================================================================
# PRE-SCRIPT
# ===========================================================================

# Parse CLI args
if [ "${1:-}" == "" ]; then
  echo "Usage: $0 [--check|--install|--cleanext]"
  echo "  --check: print installed dependencies and exit"
  echo "  --install: install missing dependencies"
  echo "  --cleanext: clean external dependencies (for reinstalling)"
  exit
fi

# Helper function to check for an executable in the system.
# ONLY CHECKS /usr/local AND /usr BY DEFAULT. If both match, gives preference to the former.
# Returns the first found match, or an empty string if there is no match.
# Usage: HAS_EXEC=$(check_exec "execname")
# $1 = exec name (e.g. "gcc")
check_exec() {
  FOUND1=$(find /usr/local/bin -name "$1" 2> /dev/null | head -n 1)
  FOUND2=$(find /usr/bin -name "$1" 2> /dev/null | head -n 1)
  if [ -n "$FOUND1" ]; then echo "$FOUND1"; elif [ -n "$FOUND2" ]; then echo "$FOUND2"; else echo ""; fi
}

# Helper function to check for a library in the system.
# ONLY CHECKS /usr/local AND /usr. If both match, gives preference to the former.
# Returns the first found match, or an empty string if there is no match.
# Usage: HAS_LIB=$(check_lib "libname")
# $1 = library name, including suffix (e.g. "libz.a")
check_lib() {
  FOUND1=$(find /usr/local/lib -name "$1" 2> /dev/null | head -n 1)
  FOUND2=$(find /usr/lib -name "$1" 2> /dev/null | head -n 1)
  if [ -n "$FOUND1" ]; then echo "$FOUND1"; elif [ -n "$FOUND2" ]; then echo "$FOUND2"; else echo ""; fi
}

# Another version of check_lib() for use with libs with multiple components (e.g. Boost).
# Returns the first found match, or an empty string if there is no match.
# Usage: HAS_LIBS=$(check_libs "libname")
# $1 = library name, including suffix (e.g. "libboost_*.a")
check_libs() {
  FOUND1=$(find /usr/local/lib -name "$1" 2> /dev/null | head -n 1)
  FOUND2=$(find /usr/lib -name "$1" 2> /dev/null | head -n 1)
  if [ -n "$FOUND1" ]; then echo "/usr/local/lib/$1"; elif [ -n "$FOUND2" ]; then echo "/usr/lib/$1"; else echo ""; fi
}

# Yet another version of check_lib() for use with header-only libs (e.g. absl)".
# Returns the first found match, or an empty string if there is no match.
# Usage: HAS_LIBS=$(check_libs_hdr "libpath")
# $1 = library folder path, including suffix (e.g. "absl")
check_libs_hdr() {
  FOUND1=$(find /usr/local/include -name "$1" 2> /dev/null | head -n 1)
  FOUND2=$(find /usr/include -name "$1" 2> /dev/null | head -n 1)
  if [ -n "$FOUND1" ]; then echo "/usr/local/include/$1"; elif [ -n "$FOUND2" ]; then echo "/usr/include/$1"; else echo ""; fi
}

# Versions for external dependencies - update numbers here if required
PROTOC_VERSION="29.3"
COMETBFT_VERSION="1.0.0"
ETHASH_VERSION="1.0.1"
EVMONE_VERSION="0.11.0"
SPEEDB_VERSION="2.8.0"

# ===========================================================================
# SCRIPT STARTS HERE
# ===========================================================================

echo "-- Scanning for dependencies..."

# Check toolchain binaries
# Necessary: git, wget, tar, gcc/g++, ld, autoconf, libtool, pkg-config, make, cmake, tmux
# Optional: ninja, mold, doxygen, clang-tidy
HAS_GIT=$(check_exec git)
HAS_WGET=$(check_exec wget)
HAS_TAR=$(check_exec tar)
HAS_GCC=$(check_exec gcc)
HAS_GPP=$(check_exec g++)
HAS_LD=$(check_exec ld)
HAS_MAKE=$(check_exec make)
HAS_CMAKE=$(check_exec cmake)
HAS_TMUX=$(check_exec tmux)
HAS_NINJA=$(check_exec ninja)
HAS_MOLD=$(check_exec mold)
HAS_DOXYGEN=$(check_exec doxygen)
HAS_CLANGTIDY=$(check_exec clang-tidy)

# Check external binaries
# Necessary: protoc, cometbft
HAS_PROTOC=$(check_exec protoc)
HAS_COMETBFT=$(check_exec cometbft)

# Check internal libraries
# Necessary: libabsl-dev, libboost-all-dev, openssl/libssl-dev, libzstd-dev, liblz4-dev, libcrypto++-dev,
#            libscrypt-dev, libc-ares-dev, libsecp256k1-dev
HAS_ABSL=$(check_libs_hdr "absl") # Required for Protobuf compilation
HAS_BOOST=$(check_libs "libboost_*.a")
HAS_LIBSSL=$(check_lib "libssl.a")
HAS_ZSTD=$(check_lib "libzstd.a")
HAS_LZ4=$(check_lib "liblz4.a")
HAS_LIBCRYPTOPP=$(check_lib "libcryptopp.a")
HAS_LIBSCRYPT=$(check_lib "libscrypt.a")
HAS_LIBCARES=$(check_lib "libcares_static.a") # Debian 13 and higher
if [ -z "$HAS_LIBCARES" ]; then HAS_LIBCARES=$(check_lib "libcares.a"); fi # Debian 12 and lower
HAS_SECP256K1=$(check_lib "libsecp256k1.a")

# Check external libraries
# Necessary: ethash (+ keccak), evmone (+ evmc), speedb
HAS_ETHASH=$(check_lib "libethash.a")
HAS_KECCAK=$(check_lib "libkeccak.a")
HAS_EVMC_INSTRUCTIONS=$(check_lib "libevmc-instructions.a")
HAS_EVMC_LOADER=$(check_lib "libevmc-loader.a")
HAS_EVMONE=$(check_lib "libevmone.a")
HAS_SPEEDB=$(check_lib "libspeedb.a")

if [ "${1:-}" == "--check" ]; then
  echo "-- Required toolchain binaries:"
  echo -n "git: " && [ -n "$HAS_GIT" ] && echo "$HAS_GIT" || echo "not found"
  echo -n "wget: " && [ -n "$HAS_WGET" ] && echo "$HAS_WGET" || echo "not found"
  echo -n "tar: " && [ -n "$HAS_TAR" ] && echo "$HAS_TAR" || echo "not found"
  echo -n "gcc: " && [ -n "$HAS_GCC" ] && echo "$HAS_GCC" || echo "not found"
  echo -n "g++: " && [ -n "$HAS_GPP" ] && echo "$HAS_GPP" || echo "not found"
  echo -n "ld: " && [ -n "$HAS_LD" ] && echo "$HAS_LD" || echo "not found"
  echo -n "make: " && [ -n "$HAS_MAKE" ] && echo "$HAS_MAKE" || echo "not found"
  echo -n "cmake: " && [ -n "$HAS_CMAKE" ] && echo "$HAS_CMAKE" || echo "not found"
  echo -n "tmux: " && [ -n "$HAS_TMUX" ] && echo "$HAS_TMUX" || echo "not found"

  echo "-- Optional toolchain binaries:"
  echo -n "ninja: " && [ -n "$HAS_NINJA" ] && echo "$HAS_NINJA" || echo "not found"
  echo -n "mold: " && [ -n "$HAS_MOLD" ] && echo "$HAS_MOLD" || echo "not found"
  echo -n "doxygen: " && [ -n "$HAS_DOXYGEN" ] && echo "$HAS_DOXYGEN" || echo "not found"
  echo -n "clang-tidy: " && [ -n "$HAS_CLANGTIDY" ] && echo "$HAS_CLANGTIDY" || echo "not found"

  echo "-- Internal libraries:"
  echo -n "absl: " && [ -n "$HAS_ABSL" ] && echo "$HAS_ABSL" || echo "not found"
  echo -n "boost: " && [ -n "$HAS_BOOST" ] && echo "$HAS_BOOST" || echo "not found"
  echo -n "libssl: " && [ -n "$HAS_LIBSSL" ] && echo "$HAS_LIBSSL" || echo "not found"
  echo -n "libzstd: " && [ -n "$HAS_ZSTD" ] && echo "$HAS_ZSTD" || echo "not found"
  echo -n "liblz4: " && [ -n "$HAS_LZ4" ] && echo "$HAS_LZ4" || echo "not found"
  echo -n "libcryptopp: " && [ -n "$HAS_LIBCRYPTOPP" ] && echo "$HAS_LIBCRYPTOPP" || echo "not found"
  echo -n "libscrypt: " && [ -n "$HAS_LIBSCRYPT" ] && echo "$HAS_LIBSCRYPT" || echo "not found"
  echo -n "libcares: " && [ -n "$HAS_LIBCARES" ] && echo "$HAS_LIBCARES" || echo "not found"
  echo -n "libsecp256k1: " && [ -n "$HAS_SECP256K1" ] && echo "$HAS_SECP256K1" || echo "not found"

  echo "-- External toolchain binaries:"
  echo -n "protoc: " && [ -n "$HAS_PROTOC" ] && echo "$HAS_PROTOC" || echo "not found"
  echo -n "cometbft: " && [ -n "$HAS_COMETBFT" ] && echo "$HAS_COMETBFT" || echo "not found"

  echo "-- External libraries:"
  echo -n "libethash: " && [ -n "$HAS_ETHASH" ] && echo "$HAS_ETHASH" || echo "not found"
  echo -n "libkeccak: " && [ -n "$HAS_KECCAK" ] && echo "$HAS_KECCAK" || echo "not found"
  echo -n "libevmc-instructions: " && [ -n "$HAS_EVMC_INSTRUCTIONS" ] && echo "$HAS_EVMC_INSTRUCTIONS" || echo "not found"
  echo -n "libevmc-loader: " && [ -n "$HAS_EVMC_LOADER" ] && echo "$HAS_EVMC_LOADER" || echo "not found"
  echo -n "libevmone: " && [ -n "$HAS_EVMONE" ] && echo "$HAS_EVMONE" || echo "not found"
  echo -n "libspeedb: " && [ -n "$HAS_SPEEDB" ] && echo "$HAS_SPEEDB" || echo "not found"
elif [ "${1:-}" == "--install" ]; then
  # Anti-anti-sudo prevention
  if [ $(id -u) -ne 0 ]; then
    echo "Please run this command as root."
    exit
  fi

  # Install binaries and internal libs (skip if not on an APT-based distro)
  HAS_APT=$(check_exec apt)
  if [ -n "$HAS_APT" ]; then
    echo "-- Checking internal dependencies..."
    PKGS=""
    if [ -z "$HAS_GIT" ]; then PKGS+="git "; fi
    if [ -z "$HAS_WGET" ]; then PKGS+="wget "; fi
    if [ -z "$HAS_TAR" ]; then PKGS+="tar "; fi
    if [ -z "$HAS_GCC" ] || [ -z "$HAS_GPP" ] || [ -z "$HAS_MAKE" ] || [ -z "$HAS_LD" ]; then PKGS+="build-essential "; fi
    if [ -z "$HAS_CMAKE" ]; then PKGS+="cmake "; fi
    if [ -z "$HAS_TMUX" ]; then PKGS+="tmux "; fi
    if [ -z "$HAS_NINJA" ]; then PKGS+="ninja-build "; fi
    if [ -z "$HAS_MOLD" ]; then PKGS+="mold "; fi
    if [ -z "$HAS_DOXYGEN" ]; then PKGS+="doxygen "; fi
    if [ -z "$HAS_CLANGTIDY" ]; then PKGS+="clang-tidy "; fi
    if [ -z "$HAS_ABSL" ]; then PKGS+="libabsl-dev "; fi
    if [ -z "$HAS_BOOST" ]; then PKGS+="libboost-all-dev "; fi
    if [ -z "$HAS_LIBSSL" ]; then PKGS+="libssl-dev "; fi
    if [ -z "$HAS_ZSTD" ]; then PKGS+="libzstd-dev "; fi
    if [ -z "$HAS_LZ4" ]; then PKGS+="liblz4-dev "; fi
    if [ -z "$HAS_LIBCRYPTOPP" ]; then PKGS+="libcrypto++-dev "; fi
    if [ -z "$HAS_LIBSCRYPT" ]; then PKGS+="libscrypt-dev "; fi
    if [ -z "$HAS_LIBCARES" ]; then PKGS+="libc-ares-dev "; fi
    if [ -z "$HAS_SECP256K1" ]; then PKGS+="libsecp256k1-dev "; fi
    if [ -n "$PKGS" ]; then
      echo "-- Installing internal dependencies..."
      apt-get install -y $PKGS
    fi
  else
    echo "-- Skipping internal dependencies (non-APT-based distro, please install those manually)"
  fi

  # Install external libs
  echo "-- Checking external dependencies..."
  if [ -z "$HAS_PROTOC" ]; then
    echo "-- Installing Protobuf..."
    cd /usr/local/src
    wget "https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOC_VERSION}/protobuf-${PROTOC_VERSION}.tar.gz"
    tar -xf "protobuf-${PROTOC_VERSION}.tar.gz"
    cd "protobuf-${PROTOC_VERSION}"
    mkdir build && cd build
    cmake -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_ABSL_PROVIDER=package -DCMAKE_INSTALL_PREFIX="/usr/local" -DCMAKE_PREFIX_PATH=/usr/include ..
    cmake --build . -- -j$(nproc) && cmake --install .
  fi
  if [ -z "$HAS_COMETBFT" ]; then
    echo "-- Installing CometBFT..."
    cd /usr/local/src
    wget "https://github.com/cometbft/cometbft/releases/download/v${COMETBFT_VERSION}/cometbft_${COMETBFT_VERSION}_linux_amd64.tar.gz"
    mkdir -p "cometbft_${COMETBFT_VERSION}_linux_amd64"
    tar -xf "cometbft_${COMETBFT_VERSION}_linux_amd64.tar.gz" -C "cometbft_${COMETBFT_VERSION}_linux_amd64"
    cp "cometbft_${COMETBFT_VERSION}_linux_amd64/cometbft" /usr/local/bin/
  fi
  if [ -z "$HAS_ETHASH" ] || [ -z "$HAS_KECCAK" ]; then
    echo "-- Installing ethash..."
    cd /usr/local/src && git clone --depth 1 --branch "v${ETHASH_VERSION}" https://github.com/chfast/ethash
    cd ethash && mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX="/usr/local" ..
    cmake --build . -- -j$(nproc) && cmake --install .
  fi
  if [ -z "$HAS_EVMC_INSTRUCTIONS" ] || [ -z "$HAS_EVMC_LOADER" ] || [ -z "$HAS_EVMONE" ]; then
    echo "-- Installing evmone..."
    cd /usr/local/src && git clone --recurse-submodules --depth 1 --branch "v${EVMONE_VERSION}" https://github.com/chfast/evmone
    cd evmone && mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX="/usr/local" -DBUILD_SHARED_LIBS=OFF -DEVMC_INSTALL=ON ..
    cmake --build . -- -j$(nproc) && cmake --install .
  fi
  if [ -z "$HAS_SPEEDB" ]; then
    echo "-- Installing speedb..."
    cd /usr/local/src && git clone --depth 1 --branch "speedb/v${SPEEDB_VERSION}" https://github.com/speedb-io/speedb
    cd speedb && mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX="/usr/local" -DCMAKE_BUILD_TYPE=Release \
      -DROCKSDB_BUILD_SHARED=OFF -DFAIL_ON_WARNINGS=OFF -DWITH_GFLAGS=OFF -DWITH_RUNTIME_DEBUG=OFF \
      -DWITH_BENCHMARK_TOOLS=OFF -DWITH_CORE_TOOLS=OFF -DWITH_TOOLS=OFF -DWITH_TRACE_TOOLS=OFF \
      -DWITH_LZ4=ON ..
    cmake --build . -- -j$(nproc) && cmake --install .
  fi
  echo "-- Dependencies installed"
elif [ "${1:-}" == "--cleanext" ]; then
  # Anti-anti-sudo prevention
  if [ $(id -u) -ne 0 ]; then
    echo "Please run this command as root."
    exit
  fi

  # Uninstall any external dependencies (+ source code repos) found in the system
  if [ -n "$HAS_PROTOC" ]; then
    echo "-- Uninstalling Protobuf..."
    rm -r "/usr/local/include/google/protobuf"
    rm "/usr/local/bin/protoc"
  fi
  if [ -n "$HAS_ETHASH" ] || [ -n "$HAS_KECCAK" ]; then
    echo "-- Uninstalling ethash..."
    rm -r "/usr/local/src/ethash"
    rm -r "/usr/local/include/ethash"
    rm "/usr/local/lib/libethash.a"
    rm "/usr/local/lib/libethash-global-context.a"
    rm "/usr/local/lib/libkeccak.a"
  fi
  if [ -n "$HAS_EVMC_INSTRUCTIONS" ] || [ -n "$HAS_EVMC_LOADER" ] || [ -n "$HAS_EVMONE" ]; then
    echo "-- Uninstalling evmone..."
    rm -r "/usr/local/src/evmone"
    rm -r "/usr/local/include/evmc"
    rm -r "/usr/local/include/evmmax"
    rm -r "/usr/local/include/evmone"
    rm "/usr/local/lib/libevmc-instructions.a"
    rm "/usr/local/lib/libevmc-loader.a"
    rm "/usr/local/lib/libevmone.a"
    rm "/usr/local/lib/libevmone-standalone.a"
  fi
  if [ -n "$HAS_SPEEDB" ]; then
    echo "-- Uninstalling speedb..."
    rm -r "/usr/local/src/speedb"
    rm -r "/usr/local/include/rocksdb"
    rm "/usr/local/lib/libspeedb.a"
  fi
  echo "-- External dependencies cleaned, please reinstall them later with --install"
fi

