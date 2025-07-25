/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef COMMON_H
#define COMMON_H

#include "../../../utils/utils.h"
#include "../../../utils/tx.h"

/// Helper struct for a worker account.
struct WorkerAccount {
  const PrivKey privKey;  ///< The account's private key.
  const Address address;  ///< The account's address.
  uint64_t nonce; ///< The account's nonce.

  /**
   * Constructor.
   * @param privKey Private key of the account.
   */
  explicit WorkerAccount (const PrivKey& privKey)
    : privKey(privKey), address(Secp256k1::toAddress(Secp256k1::toUPub(privKey))), nonce(0) {}
};

/**
 * Create a JSON request method.
 * @param method The method to create.
 * @param The params of the method.
 * @return The serialized JSON request.
 */
template <typename T> std::string makeRequestMethod(const std::string& method, const T& params) {
  return json({ {"jsonrpc", "2.0"}, {"id", 1}, {"method", method}, {"params", params} }).dump();
}

#endif // COMMON_H
