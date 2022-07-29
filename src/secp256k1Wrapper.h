#ifndef SECP256K1_H
#define SECP256K1_H

#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_recovery.h>
#include <memory>
#include "utils.h"
#include <string>


// TODO: Replace secp256k1 library with the most recent one.
// It **has** performance improvements, so this is a good idea.
// Will require compatibility patches to compile with old Aleth library
// But TransationBase will be deprecated soon, meaning the replacement will be easier
// https://github.com/bitcoin-core/secp256k1
namespace Secp256k1 {
  secp256k1_context const* getCtx();
  std::string recover(std::string& sig, std::string& messageHash);
  void appendSignature(
    const uint256_t &r, const uint256_t &s, const uint8_t &v,
    std::string &signature
  );
}

#endif  // SECP256K1_H
