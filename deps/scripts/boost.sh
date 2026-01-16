#!/bin/bash -e

BOOST_VERSION="1.83.0"
BOOST_ROOT="${DEPS_SRC}/boost"

fetch_boost() {
  echo "-- Fetching Boost..."
  git clone --depth 1 --branch "boost-${BOOST_VERSION}" https://github.com/boostorg/boost "${BOOST_ROOT}"
  cd "${BOOST_ROOT}"
  git submodule update --depth 1 --init tools/boostdep \
    libs/algorithm libs/asio libs/beast libs/chrono libs/config libs/container libs/core \
    libs/filesystem libs/multi_index libs/multiprecision libs/nowide libs/lexical_cast \
    libs/optional libs/process libs/program_options libs/thread libs/unordered
  cd "${DEPS_ROOT}"
  echo "-- Fetched Boost"
}

config_boost() {
  echo "-- Configuring Boost..."
  cd "${BOOST_ROOT}"
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" algorithm
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" asio
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" beast
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" chrono
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" config
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" container
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" core
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" filesystem
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" multi_index
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" multiprecision
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" nowide
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" lexical_cast
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" optional
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" process
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" program_options
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" thread
  python3 tools/boostdep/depinst/depinst.py -X test -g "--depth 1" unordered
  cd "${DEPS_ROOT}"
  echo "-- Configured Boost"
}

build_boost() {
  echo "-- Building Boost..."
  cd "${BOOST_ROOT}"
  ./bootstrap.sh --prefix="${DEPS_INSTALL}"
  ./b2 --no-cmake-config
  cd "${DEPS_ROOT}"
  echo "-- Built Boost"
}

install_boost() {
  echo "-- Installing Boost..."
  cd "${BOOST_ROOT}"
  ./b2 install
  cd "${DEPS_ROOT}"
  echo "-- Installed Boost"
}

clean_boost() {
  echo "-- Cleaning Boost..."
  cd "${BOOST_ROOT}"
  ./b2 clean
  cd "${DEPS_ROOT}"
  echo "-- Cleaned Boost"
}

remove_boost() {
  echo "-- Removing Boost..."
  rm -rf "${BOOST_ROOT}"
  echo "-- Removed Boost"
}

