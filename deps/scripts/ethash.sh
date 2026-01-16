#!/bin/bash -e

ETHASH_VERSION="1.1.0"
ETHASH_ROOT="${DEPS_SRC}/ethash"

fetch_ethash() {
  echo "-- Fetching Ethash..."
  git clone --depth 1 --branch "v${ETHASH_VERSION}" https://github.com/chfast/ethash "${ETHASH_ROOT}"
  echo "-- Ethash fetched"
}

config_ethash() {
  echo "-- Configuring Ethash..."
  if [ ! -d "${ETHASH_ROOT}/build" ]; then mkdir "${ETHASH_ROOT}/build"; fi
  cd "${ETHASH_ROOT}/build"
  cmake -DCMAKE_INSTALL_PREFIX="${DEPS_INSTALL}" ..
  cd "${DEPS_ROOT}"
  echo "-- Ethash configured"
}

build_ethash() {
  echo "-- Building Ethash..."
  if [ ! -d "${ETHASH_ROOT}/build" ]; then mkdir "${ETHASH_ROOT}/build"; fi
  cd "${ETHASH_ROOT}/build"
  cmake --build . -- -j$(nproc)
  cd "${DEPS_ROOT}"
  echo "-- Ethash built"
}

install_ethash() {
  echo "-- Installing Ethash..."
  if [ ! -d "${ETHASH_ROOT}/build" ]; then mkdir "${ETHASH_ROOT}/build"; fi
  cd "${ETHASH_ROOT}/build"
  cmake --install .
  cd "${DEPS_ROOT}"
  echo "-- Ethash installed"
}

clean_ethash() {
  echo "-- Cleaning Ethash..."
  if [ ! -d "${ETHASH_ROOT}/build" ]; then mkdir "${ETHASH_ROOT}/build"; fi
  cd "${ETHASH_ROOT}/build"
  cmake --build . --target clean
  cd "${DEPS_ROOT}"
  echo "-- Ethash cleaned"
}

remove_ethash() {
  echo "-- Removing Ethash..."
  rm -rf "${ETHASH_ROOT}"
  echo "-- Ethash removed"
}

