#!/bin/bash -e

COMETBFT_VERSION="0.38.19-keccak256"
COMETBFT_ROOT="${DEPS_SRC}/cometbft"

fetch_cometbft() {
  echo "-- Fetching CometBFT..."
  git clone --depth 1 --branch "v${COMETBFT_VERSION}" https://github.com/applayerlabs/cometbft "${COMETBFT_ROOT}"
  echo "-- Fetched CometBFT"
}

build_cometbft() {
  echo "-- Building CometBFT..."
  cd "${COMETBFT_ROOT}"
  make build
  cd "${DEPS_ROOT}"
  echo "-- Built CometBFT"
}

install_cometbft() {
  echo "-- Installing CometBFT..."
  if [ ! -d "${DEPS_INSTALL}/bin" ]; then mkdir -p "${DEPS_INSTALL}/bin"; fi
  cp "${COMETBFT_ROOT}/build/cometbft" "${DEPS_INSTALL}/bin/cometbft-bdk"
  echo "-- Installed CometBFT"
}

clean_cometbft() {
  echo "-- Cleaning CometBFT..."
  cd "${COMETBFT_ROOT}"
  rm -rf "${COMETBFT_ROOT}/build"
  cd "${DEPS_ROOT}"
  echo "-- Cleaned CometBFT"
}

remove_cometbft() {
  echo "-- Removing CometBFT..."
  rm -rf "${COMETBFT_ROOT}"
  echo "-- Removed CometBFT"
}

