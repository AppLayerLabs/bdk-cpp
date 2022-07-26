#ifndef TRANSACTION_H
#define TRANSACTION_H
#include "utils.h"
#include <include/web3cpp/ethcore/TransactionBase.h>
#include <secp256k1Wrapper.h>
// TODO: Error handling

namespace Tx {
  class Base {
    protected:
      // Inside RLP, TxSkeleton:
  	  Address _to;
  	  uint256_t _value;
  	  std::string _data;
  	  uint64_t _chainId;
  	  uint256_t _nonce;
  	  uint256_t _gas;
  	  uint256_t _gasPrice;

      // Secp256k1 in RLP
      uint8_t _v;
      uint256_t _r;
      uint256_t _s;

      // Outside RLP
      uint32_t _blockIndex;         // RLP + 4 BYTES
      Address _from;                // RLP + 4 BYTES + 32 BYTES
      bool callsContract = false;   // RLP + 4 BYTES + 32 BYTES + 1 BYTE
      bool inBlock = false;         // RLP + 4 BYTES + 32 BYTES + 1 BYTE + 1 BYTE
      bool hasSig = false;          // RLP + 4 BYTES + 32 BYTES + 1 BYTE + 1 BYTE + 1 BYTE 
      bool verified = false;        // RLP + 4 BYTES + 32 BYTES + 1 BYTE + 1 BYTE + 1 BYTE + 1 BYTE
                                    // TOTAL: 40 Bytes.

    public:
      // There are two ways transactions can be parsed fully from a byte string:
      // Directly from RLP (Ethereum rawTransaction), which requires to run secp256k1 to check validity and derive _from. and it is not included in a block
      // From database (RLP bytes + Outside RLP section), input from database is trusted as data will be only saved there if included in a block and is already checked.
      Base(std::string &bytes, bool fromDB);

      // You can also build your own Tx by inputting the values within the RLP Skeleton

      std::string rlpSerialize(bool includeSig);
      std::string serialize();

  };
}

#endif // TRANSACTION_H