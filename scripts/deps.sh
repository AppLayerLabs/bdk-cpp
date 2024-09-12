#!/bin/bash -e

# Parse CLI args
if [ "${1:-}" == "" ]; then
  echo "Usage: $0 [--check|--install]"
  echo "  --check: print installed dependencies and exit"
  echo "  --install: install missing dependencies"
  exit
fi

# Helper function to check for a library in the system.
# ONLY CHECKS /usr/local AND /usr. If both match, gives preference to the former.
# Usage: HAS_LIB=$(check_lib "libname")
# $1 = library name, including suffix (e.g. "libz.a")
check_lib() {
  FOUND1=$(find /usr/local/lib -name "$1" 2> /dev/null | head -n 1)
  FOUND2=$(find /usr/lib -name "$1" 2> /dev/null | head -n 1)
  if [ -n "$FOUND1" ]; then echo "$FOUND1"; elif [ -n "$FOUND2" ]; then echo "$FOUND2"; else echo ""; fi
}

# Install-specific vars
APT_PKGS=""
ETHASH_VERSION="1.0.1"
EVMONE_VERSION="0.11.0"
SPEEDB_VERSION="2.8.0"

# ===========================================================================
# SCRIPT STARTS HERE
# ===========================================================================

echo "-- Scanning for dependencies..."

# TODO: check dependency versions (if possible)

# Check toolchain binaries
# Necessary: git, gcc/g++, ld, make, cmake, tmux, protobuf-compiler (protoc), protobuf-compiler-grpc (grpc_cpp_plugin)
# Optional: ninja, mold, doxygen, clang-tidy
# TODO: which is not checking /usr/local first, this could be a problem
HAS_GIT=$(which git)
HAS_GCC=$(which gcc)
HAS_GPP=$(which g++)
HAS_LD=$(which ld)
HAS_MAKE=$(which make)
HAS_CMAKE=$(which cmake)
HAS_TMUX=$(which tmux)
HAS_PROTOC=$(which protoc)
HAS_GRPC=$(which grpc_cpp_plugin)

HAS_NINJA=$(which ninja)
HAS_MOLD=$(which mold)
HAS_DOXYGEN=$(which doxygen)
HAS_CLANGTIDY=$(which clang-tidy)

# Check internal libraries
# Necessary: libboost-all-dev, openssl/libssl-dev, libzstd-dev, libcrypto++-dev,
#            libscrypt-dev, libgrpc-dev, libgrpc++-dev, libc-ares-dev, libsecp256k1-dev
HAS_BOOST=$(check_lib "libboost_*.a") # TODO: multiple libs, handle this better
HAS_LIBSSL=$(check_lib "libssl.a")
HAS_ZSTD=$(check_lib "libzstd.a")
HAS_LIBCRYPTOPP=$(check_lib "libcryptopp.a")
HAS_LIBSCRYPT=$(check_lib "libscrypt.a")
HAS_LIBCARES=$(check_lib "libcares_static.a") # Debian 13 and higher
if [ -z "$HAS_LIBCARES" ]; then HAS_LIBCARES=$(check_lib "libcares.a"); fi # Debian 12 and lower
HAS_LIBGRPC=$(check_lib "libgrpc.a")
HAS_LIBGRPCPP=$(check_lib "libgrpc++.a")
HAS_SECP256K1=$(check_lib "libsecp256k1.a")

# Check external libraries
# Necessary: ethash (+ keccak), evmone (+ evmc), speedb
HAS_ETHASH=$(check_lib "libethash.a")
HAS_KECCAK=$(check_lib "libkeccak.a")
HAS_EVMC_INSTRUCTIONS=$(check_lib "libevmc-instructions.a")
HAS_EVMC_LOADER=$(check_lib "libevmc-loader.a")
HAS_EVMONE=$(check_lib "libevmone.a")
HAS_SPEEDB=$(check_lib "libspeedb.a")

# TODO: check library includes too(?)

if [ "${1:-}" == "--check" ]; then
  echo "-- Required toolchain binaries:"
  echo -n "git: " && [ -n "$HAS_GIT" ] && echo "$HAS_GIT" || echo "not found"
  echo -n "gcc: " && [ -n "$HAS_GCC" ] && echo "$HAS_GCC" || echo "not found"
  echo -n "g++: " && [ -n "$HAS_GPP" ] && echo "$HAS_GPP" || echo "not found"
  echo -n "ld: " && [ -n "$HAS_LD" ] && echo "$HAS_LD" || echo "not found"
  echo -n "make: " && [ -n "$HAS_MAKE" ] && echo "$HAS_MAKE" || echo "not found"
  echo -n "cmake: " && [ -n "$HAS_CMAKE" ] && echo "$HAS_CMAKE" || echo "not found"
  echo -n "tmux: " && [ -n "$HAS_TMUX" ] && echo "$HAS_TMUX" || echo "not found"
  echo -n "protoc: " && [ -n "$HAS_PROTOC" ] && echo "$HAS_PROTOC" || echo "not found"
  echo -n "grpc_cpp_plugin: " && [ -n "$HAS_GRPC" ] && echo "$HAS_GRPC" || echo "not found"

  echo "-- Optional toolchain binaries:"
  echo -n "ninja: " && [ -n "$HAS_NINJA" ] && echo "$HAS_NINJA" || echo "not found"
  echo -n "mold: " && [ -n "$HAS_MOLD" ] && echo "$HAS_MOLD" || echo "not found"
  echo -n "doxygen: " && [ -n "$HAS_DOXYGEN" ] && echo "$HAS_DOXYGEN" || echo "not found"
  echo -n "clang-tidy: " && [ -n "$HAS_CLANGTIDY" ] && echo "$HAS_CLANGTIDY" || echo "not found"

  echo "-- Internal libraries:"
  echo -n "boost: " && [ -n "$HAS_BOOST" ] && echo "$HAS_BOOST" || echo "not found"
  echo -n "libssl: " && [ -n "$HAS_LIBSSL" ] && echo "$HAS_LIBSSL" || echo "not found"
  echo -n "libzstd: " && [ -n "$HAS_ZSTD" ] && echo "$HAS_ZSTD" || echo "not found"
  echo -n "libcryptopp: " && [ -n "$HAS_LIBCRYPTOPP" ] && echo "$HAS_LIBCRYPTOPP" || echo "not found"
  echo -n "libscrypt: " && [ -n "$HAS_LIBSCRYPT" ] && echo "$HAS_LIBSCRYPT" || echo "not found"
  echo -n "libcares: " && [ -n "$HAS_LIBCARES" ] && echo "$HAS_LIBCARES" || echo "not found"
  echo -n "libgrpc: " && [ -n "$HAS_LIBGRPC" ] && echo "$HAS_LIBGRPC" || echo "not found"
  echo -n "libgrpc++: " && [ -n "$HAS_LIBGRPCPP" ] && echo "$HAS_LIBGRPCPP" || echo "not found"
  echo -n "libsecp256k1: " && [ -n "$HAS_SECP256K1" ] && echo "$HAS_SECP256K1" || echo "not found"

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

  # Install binaries and internal libs
  # TODO: this works on APT distros only for now
  echo "-- Checking internal dependencies..."
  if [ -z "$HAS_GIT" ]; then APT_PKGS+="git "; fi
  if [ -z "$HAS_GCC" ] || [ -z "$HAS_GPP" ] || [ -z "$HAS_MAKE" ] || [ -z "$HAS_LD" ]; then APT_PGKS+="build-essential "; fi
  if [ -z "$HAS_CMAKE" ]; then APT_PKGS+="cmake "; fi
  if [ -z "$HAS_TMUX" ]; then APT_PKGS+="tmux "; fi
  if [ -z "$HAS_PROTOC" ]; then APT_PKGS+="protobuf-compiler "; fi
  if [ -z "$HAS_GRPC" ]; then APT_PKGS+="protobuf-compiler-grpc "; fi
  if [ -z "$HAS_NINJA" ]; then APT_PKGS+="ninja-build "; fi
  if [ -z "$HAS_MOLD" ]; then APT_PKGS+="mold "; fi
  if [ -z "$HAS_DOXYGEN" ]; then APT_PKGS+="doxygen "; fi
  if [ -z "$HAS_CLANGTIDY" ]; then APT_PKGS+="clang-tidy "; fi
  if [ -z "$HAS_BOOST" ]; then APT_PKGS+="libboost-all-dev "; fi
  if [ -z "$HAS_LIBSSL" ]; then APT_PKGS+="libssl-dev "; fi
  if [ -z "$HAS_ZSTD" ]; then APT_PKGS+="libzstd-dev "; fi
  if [ -z "$HAS_LIBCRYPTOPP" ]; then APT_PKGS+="libcrypto++-dev "; fi
  if [ -z "$HAS_LIBSCRYPT" ]; then APT_PKGS+="libscrypt-dev "; fi
  if [ -z "$HAS_LIBCARES" ]; then APT_PKGS+="libc-ares-dev "; fi
  if [ -z "$HAS_LIBGRPC" ]; then APT_PKGS+="libgrpc-dev "; fi
  if [ -z "$HAS_LIBGRPCPP" ]; then APT_PKGS+="libgrpc++-dev "; fi
  if [ -z "$HAS_SECP256K1" ]; then APT_PKGS+="libsecp256k1-dev "; fi
  if [ -n "$APT_PKGS" ]; then
    echo "-- Installing internal dependencies..."
    apt-get install -y $APT_PKGS
  fi

  # Install external libs
  # TODO: maybe parametrize -j$(nproc)?
  echo "-- Checking external dependencies..."
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
      -DWITH_BENCHMARK_TOOLS=OFF -DWITH_CORE_TOOLS=OFF -DWITH_TOOLS=OFF -DWITH_TRACE_TOOLS=OFF ..
    cmake --build . -- -j$(nproc) && cmake --install .
  fi
  echo "-- Dependencies installed"
fi

