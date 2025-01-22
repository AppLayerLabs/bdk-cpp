#ifndef _CORE_H_
#define _CORE_H_

#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>

#include "../utils/strings.h"
#include "../utils/safehash.h"

/// Mempool model to help validate multiple txs with same from account and various nonce values.
using MempoolModel =
  std::unordered_map<
    Address, // From Address -->
    std::map<
      uint64_t, // Nonce (of from address) -->
      std::unordered_map<
        Hash, // Transaction hash (for a from address and nonce) -->
        std::pair<
          uint256_t, // Minimum balance (max fee) required by this transaction.
          bool // Eject? `true` if tx should be removed from the mempool on next CheckTx().
        >
      , SafeHash>
    >
  , SafeHash>;

using MempoolModelIt = MempoolModel::iterator;

using MempoolModelNonceIt =
  std::map<
    uint64_t,
    std::unordered_map<
      Hash,
      std::pair<
        uint256_t,
        bool>
      , SafeHash>
  , SafeHash>::iterator;

using MempoolModelHashIt =
  std::unordered_map<
    Hash,
    std::pair<
      uint256_t,
      bool
    >
  , SafeHash>::iterator;


#endif // _CORE_H_