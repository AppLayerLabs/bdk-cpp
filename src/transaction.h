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
  	  uint256_t _value = 0;
  	  std::string _data = "";
  	  uint64_t _chainId = 0;
  	  uint256_t _nonce = 0;
  	  uint256_t _gas = 0;
  	  uint256_t _gasPrice = 0;

      // Secp256k1 in RLP
      uint256_t _v = 0;
      uint256_t _r = 0;
      uint256_t _s = 0;

      // Outside RLP
      uint32_t _blockIndex = 0;          // RLP + 4 BYTES Where on the block the TX is located.
      Address _from;                 // RLP + 4 BYTES + 32 BYTES
      bool _callsContract = false;   // RLP + 4 BYTES + 32 BYTES + 1 BYTE
      bool _inBlock = false;         // RLP + 4 BYTES + 32 BYTES + 1 BYTE + 1 BYTE
      bool _hasSig = false;          // RLP + 4 BYTES + 32 BYTES + 1 BYTE + 1 BYTE + 1 BYTE 
      bool _verified = false;        // RLP + 4 BYTES + 32 BYTES + 1 BYTE + 1 BYTE + 1 BYTE + 1 BYTE
                                     // TOTAL: 40 Bytes.

    public:
      // There are two ways transactions can be parsed fully from a byte string:
      // Directly from RLP (Ethereum rawTransaction), which requires to run secp256k1 to check validity and derive _from. and it is not included in a block
      // From database (RLP bytes + Outside RLP section), input from database is trusted as data will be only saved there if included in a block and is already checked.
      Base(std::string &bytes, bool fromDB);
    

      // You can also build your own Tx by inputting the values within the RLP Skeleton
      Base(Address &from, Address &to, uint256_t &value, std::string &data, uint64_t &chainId, uint256_t &nonce, uint256_t &gas, uint256_t &gasPrice) :
        _from(from), _to(to), _value(value), _data(data), _chainId(chainId), _nonce(nonce), _gas(gas), _gasPrice(gasPrice) { }
      
      
      // Getters
      const Address& to() { return _to; };
      const uint256_t& value() { return _value; };
      const std::string& data() { return _data; };
      const uint64_t& chainId() { return _chainId; };
      const uint256_t& nonce() { return _nonce; };
      const uint256_t& gas() { return _gas; };
      const uint256_t& gasPrice() { return _gasPrice; };
      const uint256_t& v() { return _v; };
      const uint256_t& r() { return _r; };
      const uint256_t& s() { return _s; };
      const uint32_t& blockIndex() { return _blockIndex; };
      const Address& from() { return _from; };
      const bool& callsContract() { return _callsContract; };
      const bool& inBlock() { return _inBlock; };
      const bool& hasSig() { return _hasSig; };
      const bool& verified() { return _verified; };

      std::string rlpSerialize(bool includeSig);
      std::string serialize();
      std::string hash();

  };
}

#endif // TRANSACTION_H