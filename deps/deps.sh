#!/bin/bash -e

DEPS_ROOT="$(dirname "$(realpath "$0")")"
DEPS_SRC="${DEPS_ROOT}/src"
DEPS_INSTALL="${DEPS_ROOT}/target"

if [ "${1:-}" == "" ] || [ "${2:-}" == "" ] ; then
  echo "Usage: $0 <command> all|<module>"
  echo "  Commands: fetch, config, build, install, clean, remove"
  echo "  Modules: boost, cometbft, cryptopp, ethash, evmone, openssl, protobuf, rocksdb, secp256k1, sqlitecpp"
  exit
fi

source "${DEPS_ROOT}/scripts/boost.sh"
source "${DEPS_ROOT}/scripts/cometbft.sh"
source "${DEPS_ROOT}/scripts/cryptopp.sh"
source "${DEPS_ROOT}/scripts/ethash.sh"
source "${DEPS_ROOT}/scripts/evmone.sh" # Includes ethash and intx (via Hunter)
source "${DEPS_ROOT}/scripts/openssl.sh"
source "${DEPS_ROOT}/scripts/protobuf.sh" # Includes absl
source "${DEPS_ROOT}/scripts/rocksdb.sh"
source "${DEPS_ROOT}/scripts/secp256k1.sh"
source "${DEPS_ROOT}/scripts/sqlitecpp.sh"

if [ "${1:-}" == "fetch" ]; then
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "boost" ] ; then fetch_boost; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "cometbft" ] ; then fetch_cometbft; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "cryptopp" ] ; then fetch_cryptopp; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "ethash" ] ; then fetch_ethash; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "evmone" ] ; then fetch_evmone; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "openssl" ] ; then fetch_openssl; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "protobuf" ] ; then fetch_protobuf; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "rocksdb" ] ; then fetch_rocksdb; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "secp256k1" ] ; then fetch_secp256k1; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "sqlitecpp" ] ; then fetch_sqlitecpp; fi
elif [ "${1:-}" == "config" ]; then
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "boost" ] ; then config_boost; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "ethash" ] ; then config_ethash; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "evmone" ] ; then config_evmone; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "openssl" ] ; then config_openssl; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "protobuf" ] ; then config_protobuf; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "rocksdb" ] ; then config_rocksdb; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "secp256k1" ] ; then config_secp256k1; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "sqlitecpp" ] ; then config_sqlitecpp; fi
elif [ "${1:-}" == "build" ]; then
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "boost" ] ; then build_boost; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "cometbft" ] ; then build_cometbft; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "cryptopp" ] ; then build_cryptopp; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "ethash" ] ; then build_ethash; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "evmone" ] ; then build_evmone; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "openssl" ] ; then build_openssl; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "protobuf" ] ; then build_protobuf; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "rocksdb" ] ; then build_rocksdb; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "secp256k1" ] ; then build_secp256k1; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "sqlitecpp" ] ; then build_sqlitecpp; fi
elif [ "${1:-}" == "install" ]; then
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "boost" ] ; then install_boost; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "cometbft" ] ; then install_cometbft; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "cryptopp" ] ; then install_cryptopp; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "ethash" ] ; then install_ethash; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "evmone" ] ; then install_evmone; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "openssl" ] ; then install_openssl; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "protobuf" ] ; then install_protobuf; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "rocksdb" ] ; then install_rocksdb; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "secp256k1" ] ; then install_secp256k1; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "sqlitecpp" ] ; then install_sqlitecpp; fi
elif [ "${1:-}" == "clean" ]; then
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "boost" ] ; then clean_boost; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "cometbft" ] ; then clean_cometbft; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "cryptopp" ] ; then clean_cryptopp; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "ethash" ] ; then clean_ethash; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "evmone" ] ; then clean_evmone; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "openssl" ] ; then clean_openssl; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "protobuf" ] ; then clean_protobuf; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "rocksdb" ] ; then clean_rocksdb; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "secp256k1" ] ; then clean_secp256k1; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "sqlitecpp" ] ; then clean_sqlitecpp; fi
elif [ "${1:-}" == "remove" ]; then
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "boost" ] ; then remove_boost; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "cometbft" ] ; then remove_cometbft; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "cryptopp" ] ; then remove_cryptopp; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "ethash" ] ; then remove_ethash; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "evmone" ] ; then remove_evmone; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "openssl" ] ; then remove_openssl; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "protobuf" ] ; then remove_protobuf; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "rocksdb" ] ; then remove_rocksdb; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "secp256k1" ] ; then remove_secp256k1; fi
  if [ "${2:-}" == "all" ] || [ "${2:-}" == "sqlitecpp" ] ; then remove_sqlitecpp; fi
fi

