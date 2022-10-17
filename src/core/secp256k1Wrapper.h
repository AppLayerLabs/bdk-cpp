#ifndef SECP256K1_WRAPPER_H
#define SECP256K1_WRAPPER_H

#include <memory>
#include <string>

#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_recovery.h>

#include "utils.h"

namespace Secp256k1 {
  UncompressedPubkey recover(const Signature& sig, const Hash& messageHash);
  Signature appendSignature(const uint256_t &r, const uint256_t &s, const uint8_t &v);
  UncompressedPubkey toPub(const PrivKey &privKey);
  UncompressedPubkey toPub(const CompressedPubkey &pubKey);
  CompressedPubkey toPubCompressed(const PrivKey &privKey);
  Address toAddress(const UncompressedPubkey &pubKey);
  Address toAddress(const CompressedPubkey &pubKey);
  Signature sign(const PrivKey &privKey, const Hash &hash);
  bool verify(const UncompressedPubkey& pubkey, const Signature& sig, const Hash& msghash);
}

#endif  // SECP256K1_WRAPPER_H
