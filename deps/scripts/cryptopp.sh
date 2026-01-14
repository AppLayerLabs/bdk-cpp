#!/bin/bash -e

CRYPTOPP_VERSION="8_9_0"
CRYPTOPP_ROOT="${DEPS_SRC}/cryptopp"

fetch_cryptopp() {
  echo "-- Fetching CryptoPP..."
  git clone --depth 1 --branch "CRYPTOPP_${CRYPTOPP_VERSION}" https://github.com/weidai11/cryptopp "${CRYPTOPP_ROOT}"
  echo "-- CryptoPP fetched"
}

build_cryptopp() {
  echo "-- Building CryptoPP..."
  cd "${CRYPTOPP_ROOT}"
  make static
  cd "${DEPS_ROOT}"
  echo "-- CryptoPP built"
}

install_cryptopp() {
  echo "-- Installing CryptoPP..."
  cd "${CRYPTOPP_ROOT}"
  make install PREFIX="${DEPS_INSTALL}"
  cd "${DEPS_ROOT}"
  echo "-- CryptoPP installed"
}

clean_cryptopp() {
  echo "-- Cleaning CryptoPP..."
  cd "${CRYPTOPP_ROOT}"
  make clean
  cd "${DEPS_ROOT}"
  echo "-- CryptoPP cleaned"
}

remove_cryptopp() {
  echo "-- Removing CryptoPP..."
  rm -rf "${CRYPTOPP_ROOT}"
  echo "-- CryptoPP removed"
}

