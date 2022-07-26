#ifndef SECP256K1_H
#define SECP256K1_H

#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_recovery.h>
#include <secp256k1_sha256.h>
#include <memory>
#include "utils.h"
#include <string>

namespace Secp256k1 {
  
  // Tries to recover a public key from signature and signature hash (in bytes)
  std::string recover(std::string sig, std::string messageHash);
}



#endif