#!/bin/bash -e

# ===========================================================================
# PRE-SCRIPT
# ===========================================================================

# Parse CLI args
if [ "${1:-}" == "" ]; then
  echo "Usage: $0 [--check|--install]"
  echo "  --check: print installed dependencies and exit"
  echo "  --install: install missing dependencies"
  exit
fi

# Helper function to check for an executable in the system.
# ONLY CHECKS /usr/local AND /usr BY DEFAULT. If both match, gives preference to the former.
# Pass an extra path to it ($2) so it can look there.
# Returns the first found match, or an empty string if there is no match.
# Usage: HAS_EXEC=$(check_exec "execname")
# $1 = exec name (e.g. "gcc")
# $2 = optional extra path to look for outside of the defaults (e.g. "/usr/local/gcc*")
check_exec() {
  if [ -n "$2" ]; then
    FOUND0=$(find $2 -name "$1" 2> /dev/null | head -n 1)
    if [ -n "$FOUND0" ]; then echo "$FOUND0"; return; fi
  fi
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

# Versions for external dependencies - update numbers here if required
ETHASH_VERSION="1.0.1"
EVMONE_VERSION="0.11.0"
SPEEDB_VERSION="2.8.0"

# ===========================================================================
# SCRIPT STARTS HERE
# ===========================================================================

echo "-- Scanning for dependencies..."

# Check toolchain binaries
# Necessary: git, gcc/g++, ld, make, cmake, tmux, protobuf-compiler (protoc), protobuf-compiler-grpc (grpc_cpp_plugin)
# Optional: ninja, mold, doxygen, clang-tidy
HAS_GIT=$(check_exec git)
HAS_GCC=$(check_exec gcc "/usr/local/gcc*")
HAS_GPP=$(check_exec g++ "/usr/local/gcc*")
HAS_LD=$(check_exec ld)
HAS_MAKE=$(check_exec make)
HAS_CMAKE=$(check_exec cmake)
HAS_TMUX=$(check_exec tmux)
HAS_PROTOC=$(check_exec protoc)
HAS_GRPC=$(check_exec grpc_cpp_plugin)

HAS_NINJA=$(check_exec ninja)
HAS_MOLD=$(check_exec mold)
HAS_DOXYGEN=$(check_exec doxygen)
HAS_CLANGTIDY=$(check_exec clang-tidy)

# Check internal libraries
# Necessary: libboost-all-dev, openssl/libssl-dev, libzstd-dev, libcrypto++-dev,
#            libscrypt-dev, libgrpc-dev, libgrpc++-dev, libc-ares-dev, libsecp256k1-dev
HAS_BOOST=$(check_libs "libboost_*.a")
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
  PKGS=""
  if [ -z "$HAS_GIT" ]; then PKGS+="git "; fi
  if [ -z "$HAS_GCC" ] || [ -z "$HAS_GPP" ] || [ -z "$HAS_MAKE" ] || [ -z "$HAS_LD" ]; then PGKS+="build-essential "; fi
  if [ -z "$HAS_CMAKE" ]; then PKGS+="cmake "; fi
  if [ -z "$HAS_TMUX" ]; then PKGS+="tmux "; fi
  if [ -z "$HAS_PROTOC" ]; then PKGS+="protobuf-compiler "; fi
  if [ -z "$HAS_GRPC" ]; then PKGS+="protobuf-compiler-grpc "; fi
  if [ -z "$HAS_NINJA" ]; then PKGS+="ninja-build "; fi
  if [ -z "$HAS_MOLD" ]; then PKGS+="mold "; fi
  if [ -z "$HAS_DOXYGEN" ]; then PKGS+="doxygen "; fi
  if [ -z "$HAS_CLANGTIDY" ]; then PKGS+="clang-tidy "; fi
  if [ -z "$HAS_BOOST" ]; then PKGS+="libboost-all-dev "; fi
  if [ -z "$HAS_LIBSSL" ]; then PKGS+="libssl-dev "; fi
  if [ -z "$HAS_ZSTD" ]; then PKGS+="libzstd-dev "; fi
  if [ -z "$HAS_LIBCRYPTOPP" ]; then PKGS+="libcrypto++-dev "; fi
  if [ -z "$HAS_LIBSCRYPT" ]; then PKGS+="libscrypt-dev "; fi
  if [ -z "$HAS_LIBCARES" ]; then PKGS+="libc-ares-dev "; fi
  if [ -z "$HAS_LIBGRPC" ]; then PKGS+="libgrpc-dev "; fi
  if [ -z "$HAS_LIBGRPCPP" ]; then PKGS+="libgrpc++-dev "; fi
  if [ -z "$HAS_SECP256K1" ]; then PKGS+="libsecp256k1-dev "; fi
  if [ -n "$PKGS" ]; then
    echo "-- Installing internal dependencies..."
    apt-get install -y $PKGS
  fi

  # Install external libs
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

