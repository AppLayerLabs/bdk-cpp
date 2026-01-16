#!/bin/bash -e

PROTOBUF_VERSION="33.3"
PROTOBUF_ROOT="${DEPS_SRC}/protobuf"

fetch_protobuf() {
  echo "-- Fetching Protobuf..."
  git clone --depth 1 --branch "v${PROTOBUF_VERSION}" https://github.com/protocolbuffers/protobuf "${PROTOBUF_ROOT}"
  echo "-- Protobuf fetched"
}

config_protobuf() {
  echo "-- Configuring Protobuf..."
  if [ ! -d "${PROTOBUF_ROOT}/build" ]; then mkdir "${PROTOBUF_ROOT}/build"; fi
  cd "${PROTOBUF_ROOT}/build"
  cmake -DCMAKE_INSTALL_PREFIX="${DEPS_INSTALL}" -DCMAKE_INSTALL_LIBDIR="${DEPS_INSTALL}/lib" \
    -Dprotobuf_BUILD_TESTS=OFF ..
  cd "${DEPS_ROOT}"
  echo "-- Protobuf configured"
}

build_protobuf() {
  echo "-- Building Protobuf..."
  if [ ! -d "${PROTOBUF_ROOT}/build" ]; then mkdir "${PROTOBUF_ROOT}/build"; fi
  cd "${PROTOBUF_ROOT}/build"
  cmake --build . -- -j$(nproc)
  cd "${DEPS_ROOT}"
  echo "-- Protobuf built"
}

install_protobuf() {
  echo "-- Installing Protobuf..."
  if [ ! -d "${PROTOBUF_ROOT}/build" ]; then mkdir "${PROTOBUF_ROOT}/build"; fi
  cd "${PROTOBUF_ROOT}/build"
  cmake --install .
  cd "${DEPS_ROOT}"
  echo "-- Protobuf installed"
}

clean_protobuf() {
  echo "-- Cleaning Protobuf..."
  if [ ! -d "${PROTOBUF_ROOT}/build" ]; then mkdir "${PROTOBUF_ROOT}/build"; fi
  cd "${PROTOBUF_ROOT}/build"
  cmake --build . --target clean
  cd "${DEPS_ROOT}"
  echo "-- Protobuf cleaned"
}

remove_protobuf() {
  echo "-- Removing Protobuf..."
  rm -rf "${PROTOBUF_ROOT}"
  echo "-- Protobuf removed"
}

