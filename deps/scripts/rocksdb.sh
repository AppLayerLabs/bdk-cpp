#!/bin/bash -e

ROCKSDB_VERSION="10.9.1"
ROCKSDB_ROOT="${DEPS_SRC}/rocksdb"

fetch_rocksdb() {
  echo "-- Fetching RocksDB..."
  git clone --depth 1 --branch "v${ROCKSDB_VERSION}" https://github.com/facebook/rocksdb "${ROCKSDB_ROOT}"
  echo "-- RocksDB fetched"
}

config_rocksdb() {
  echo "-- Configuring RocksDB..."
  if [ ! -d "${ROCKSDB_ROOT}/build" ]; then mkdir "${ROCKSDB_ROOT}/build"; fi
  cd "${ROCKSDB_ROOT}/build"
  cmake -DCMAKE_INSTALL_PREFIX="${DEPS_INSTALL}" -DCMAKE_BUILD_TYPE=Release \
    -DROCKSDB_BUILD_SHARED=OFF -DFAIL_ON_WARNINGS=OFF -DWITH_GFLAGS=OFF -DWITH_RUNTIME_DEBUG=OFF \
    -DWITH_BENCHMARK_TOOLS=OFF -DWITH_CORE_TOOLS=OFF -DWITH_TOOLS=OFF -DWITH_TRACE_TOOLS=OFF \
    -DWITH_LZ4=ON ..
  cd "${DEPS_ROOT}"
  echo "-- RocksDB configured"
}

build_rocksdb() {
  echo "-- Building RocksDB..."
  if [ ! -d "${ROCKSDB_ROOT}/build" ]; then mkdir "${ROCKSDB_ROOT}/build"; fi
  cd "${ROCKSDB_ROOT}/build"
  cmake --build . -- -j$(nproc)
  cd "${DEPS_ROOT}"
  echo "-- RocksDB built"
}

install_rocksdb() {
  echo "-- Installing RocksDB..."
  if [ ! -d "${ROCKSDB_ROOT}/build" ]; then mkdir "${ROCKSDB_ROOT}/build"; fi
  cd "${ROCKSDB_ROOT}/build"
  cmake --install .
  cd "${DEPS_ROOT}"
  echo "-- RocksDB installed"
}

clean_rocksdb() {
  echo "-- Cleaning RocksDB..."
  if [ ! -d "${ROCKSDB_ROOT}/build" ]; then mkdir "${ROCKSDB_ROOT}/build"; fi
  cd "${ROCKSDB_ROOT}/build"
  cmake --build . --target clean
  cd "${DEPS_ROOT}"
  echo "-- RocksDB cleaned"
}

remove_rocksdb() {
  echo "-- Removing RocksDB..."
  rm -rf "${ROCKSDB_ROOT}"
  echo "-- RocksDB removed"
}

