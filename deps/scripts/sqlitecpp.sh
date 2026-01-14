#!/bin/bash -e

SQLITECPP_VERSION="3.3.3"
SQLITECPP_ROOT="${DEPS_SRC}/sqlitecpp"

fetch_sqlitecpp() {
  echo "-- Fetching SQLiteCPP..."
  git clone --depth 1 --branch "${SQLITECPP_VERSION}" https://github.com/SRombauts/SQLiteCpp "${SQLITECPP_ROOT}"
  echo "-- SQLiteCPP fetched"
}

config_sqlitecpp() {
  echo "-- Configuring SQLiteCPP..."
  if [ ! -d "${SQLITECPP_ROOT}/build" ]; then mkdir "${SQLITECPP_ROOT}/build"; fi
  cd "${SQLITECPP_ROOT}/build"
  cmake -DCMAKE_INSTALL_PREFIX="${DEPS_INSTALL}" -DCMAKE_BUILD_TYPE=Release ..
  cd "${DEPS_ROOT}"
  echo "-- SQLiteCPP configured"
}

build_sqlitecpp() {
  echo "-- Building SQLiteCPP..."
  if [ ! -d "${SQLITECPP_ROOT}/build" ]; then mkdir "${SQLITECPP_ROOT}/build"; fi
  cd "${SQLITECPP_ROOT}/build"
  cmake --build . -- -j$(nproc)
  cd "${DEPS_ROOT}"
  echo "-- SQLiteCPP built"
}

install_sqlitecpp() {
  echo "-- Installing SQLiteCPP..."
  if [ ! -d "${SQLITECPP_ROOT}/build" ]; then mkdir "${SQLITECPP_ROOT}/build"; fi
  cd "${SQLITECPP_ROOT}/build"
  cmake --install .
  cd "${DEPS_ROOT}"
  echo "-- SQLiteCPP installed"
}

clean_sqlitecpp() {
  echo "-- Cleaning SQLiteCPP..."
  if [ ! -d "${SQLITECPP_ROOT}/build" ]; then mkdir "${SQLITECPP_ROOT}/build"; fi
  cd "${SQLITECPP_ROOT}/build"
  cmake --build . --target clean
  cd "${DEPS_ROOT}"
  echo "-- SQLiteCPP cleaned"
}

remove_sqlitecpp() {
  echo "-- Removing SQLiteCPP..."
  rm -rf "${SQLITECPP_ROOT}"
  echo "-- SQLiteCPP removed"
}

