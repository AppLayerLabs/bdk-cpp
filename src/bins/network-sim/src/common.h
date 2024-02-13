#ifndef COMMON_H
#define COMMON_H

#include "../../../utils/utils.h"
#include "../../../utils/tx.h"

struct WorkerAccount {
  const PrivKey privKey;
  const Address address;
  uint64_t nonce;
  explicit WorkerAccount (const PrivKey& privKey) : privKey(privKey), address(Secp256k1::toAddress(Secp256k1::toUPub(privKey))), nonce(0) {}
};

template <typename T>
std::string makeRequestMethod(const std::string& method, const T& params) {
  return json({
    {"jsonrpc", "2.0"},
    {"id", 1},
    {"method", method},
    {"params", params}
  }).dump();
}

#endif // COMMON_H