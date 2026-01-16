#!/bin/bash -e

OPENSSL_VERSION="3.6.0"
OPENSSL_ROOT="${DEPS_SRC}/openssl"

fetch_openssl() {
  echo "-- Fetching OpenSSL..."
  git clone --depth 1 --branch "openssl-${OPENSSL_VERSION}" https://github.com/openssl/openssl "${OPENSSL_ROOT}"
  echo "-- OpenSSL fetched"
}

config_openssl() {
  echo "-- Configuring OpenSSL..."
  cd "${OPENSSL_ROOT}"
  ./Configure --prefix="${DEPS_INSTALL}" --libdir="${DEPS_INSTALL}/lib" --openssldir="${DEPS_INSTALL}"
  cd "${DEPS_ROOT}"
  echo "-- OpenSSL configured"
}

build_openssl() {
  echo "-- Building OpenSSL..."
  cd "${OPENSSL_ROOT}"
  make
  cd "${DEPS_ROOT}"
  echo "-- OpenSSL built"
}

install_openssl() {
  echo "-- Installing OpenSSL..."
  cd "${OPENSSL_ROOT}"
  make install
  cd "${DEPS_ROOT}"
  echo "-- OpenSSL installed"
}

clean_openssl() {
  echo "-- Cleaning OpenSSL..."
  cd "${OPENSSL_ROOT}"
  make clean
  cd "${DEPS_ROOT}"
  echo "-- OpenSSL cleaned"
}

remove_openssl() {
  echo "-- Removing OpenSSL..."
  rm -rf "${OPENSSL_ROOT}"
  echo "-- OpenSSL removed"
}

