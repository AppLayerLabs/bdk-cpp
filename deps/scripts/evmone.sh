#!/bin/bash -e

EVMONE_VERSION="0.18.0"
EVMONE_ROOT="${DEPS_SRC}/evmone"

fetch_evmone() {
  echo "-- Fetching EVMOne..."
  git clone --recurse-submodules --depth 1 --branch "v${EVMONE_VERSION}" https://github.com/ipsilon/evmone "${EVMONE_ROOT}"
  echo "-- EVMOne fetched"
}

# HACK: EVMOne can't build both static and shared libs at the same time, and its testing suite requires shared libs for some reason.
# We're "forced" to configure and compile this thing twice: once for testing, and another for the build itself that will be used in BDK.
# So the testing compile (and running the tests in itself) is done as a "config" command. Sloppy, but it works.

config_evmone() {
  echo "-- Configuring EVMOne..."
  if [ ! -d "${EVMONE_ROOT}/build" ]; then mkdir "${EVMONE_ROOT}/build"; fi
  cd "${EVMONE_ROOT}/build"
  cmake -DCMAKE_INSTALL_PREFIX="${DEPS_INSTALL}" -DCMAKE_INSTALL_LIBDIR="${DEPS_INSTALL}/lib" \
    -DBUILD_SHARED_LIBS=ON -DEVMC_INSTALL=ON -DEVMONE_TESTING=ON ..
  cmake --build . -- -j$(nproc)
  ./bin/evmc-vmtester ./lib64/libevmone.so && ./bin/evmone-unittests
  cd "${DEPS_ROOT}"
  echo "-- EVMOne configured"
}

build_evmone() {
  echo "-- Building EVMOne..."
  if [ ! -d "${EVMONE_ROOT}/build" ]; then mkdir "${EVMONE_ROOT}/build"; fi
  cd "${EVMONE_ROOT}/build"
  cmake -DCMAKE_INSTALL_PREFIX="${DEPS_INSTALL}" -DCMAKE_INSTALL_LIBDIR="${DEPS_INSTALL}/lib" \
    -DBUILD_SHARED_LIBS=OFF -DEVMC_INSTALL=ON ..
  cmake --build . -- -j$(nproc)
  cd "${DEPS_ROOT}"
  echo "-- EVMOne built"
}

install_evmone() {
  echo "-- Installing EVMOne..."
  if [ ! -d "${EVMONE_ROOT}/build" ]; then mkdir "${EVMONE_ROOT}/build"; fi
  cd "${EVMONE_ROOT}/build"
  cmake --install .
  cd "${DEPS_ROOT}"
  echo "-- EVMOne installed"
}

clean_evmone() {
  echo "-- Cleaning EVMOne..."
  if [ ! -d "${EVMONE_ROOT}/build" ]; then mkdir "${EVMONE_ROOT}/build"; fi
  cd "${EVMONE_ROOT}/build"
  cmake --build . --target clean
  cd "${DEPS_ROOT}"
  echo "-- EVMOne cleaned"
}

remove_evmone() {
  echo "-- Removing EVMOne..."
  rm -rf "${EVMONE_ROOT}"
  echo "-- EVMOne removed"
}

