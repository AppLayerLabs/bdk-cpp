#ifndef SECP256K1_WRAPPER_H
#define SECP256K1_WRAPPER_H

#include <memory>
#include <string>

#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_recovery.h>

#include "utils.h"

namespace Secp256k1 {
  std::string recover(const std::string& sig, const std::string& messageHash);
  void appendSignature(
    const uint256_t &r, const uint256_t &s, const uint8_t &v,
    std::string &signature
  );
  std::string toPub(const std::string &privKey);
  std::string toAddress(const std::string &pubKey);
  std::string sign(const std::string &privKey, const std::string &hash);
}

#endif  // SECP256K1_WRAPPER_H
