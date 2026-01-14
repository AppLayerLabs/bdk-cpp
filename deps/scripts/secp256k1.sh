#!/bin/bash -e

SECP256K1_VERSION="0.7.0"
SECP256K1_ROOT="${DEPS_SRC}/secp256k1"

fetch_secp256k1() {
  echo "-- Fetching Secp256k1..."
  git clone --depth 1 --branch "v${SECP256K1_VERSION}" https://github.com/bitcoin-core/secp256k1 "${SECP256K1_ROOT}"
  echo "-- Secp256k1 fetched"
}

config_secp256k1() {
  echo "-- Configuring Secp256k1..."
  if [ ! -d "${SECP256K1_ROOT}/build" ]; then mkdir "${SECP256K1_ROOT}/build"; fi
  cd "${SECP256K1_ROOT}/build"
  cmake -DSECP256K1_DISABLE_SHARED=ON -DSECP256K1_ENABLE_MODULE_RECOVERY=ON \
    -DSECP256K1_BUILD_BENCHMARK=OFF -DSECP256K1_BUILD_TESTS=OFF \
    -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=OFF -DSECP256K1_BUILD_CTIME_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${DEPS_INSTALL}" ..
  cd "${DEPS_ROOT}"
  echo "-- Secp256k1 configured"
}

build_secp256k1() {
  echo "-- Building Secp256k1..."
  if [ ! -d "${SECP256K1_ROOT}/build" ]; then mkdir "${SECP256K1_ROOT}/build"; fi
  cd "${SECP256K1_ROOT}/build"
  cmake --build . -- -j$(nproc)
  cd "${DEPS_ROOT}"
  echo "-- Secp256k1 built"
}

install_secp256k1() {
  echo "-- Installing Secp256k1..."
  if [ ! -d "${SECP256K1_ROOT}/build" ]; then mkdir "${SECP256K1_ROOT}/build"; fi
  cd "${SECP256K1_ROOT}/build"
  cmake --install .
  cd "${DEPS_ROOT}"
  echo "-- Secp256k1 installed"
}

clean_secp256k1() {
  echo "-- Cleaning Secp256k1..."
  if [ ! -d "${SECP256K1_ROOT}/build" ]; then mkdir "${SECP256K1_ROOT}/build"; fi
  cd "${SECP256K1_ROOT}/build"
  cmake --build . --target clean
  cd "${DEPS_ROOT}"
  echo "-- Secp256k1 cleaned"
}

remove_secp256k1() {
  echo "-- Removing Secp256k1..."
  rm -rf "${SECP256K1_ROOT}"
  echo "-- Secp256k1 removed"
}

