#ifndef SECP256K1_WRAPPER_H
#define SECP256K1_WRAPPER_H

#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_recovery.h>
#include <memory>
#include "utils.h"
#include <string>

namespace Secp256k1 {
  std::string recover(std::string& sig, std::string& messageHash);
  void appendSignature(
    const uint256_t &r, const uint256_t &s, const uint8_t &v,
    std::string &signature
  );
}

#endif  // SECP256K1_WRAPPER_H
