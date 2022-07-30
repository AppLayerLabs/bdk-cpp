#ifndef TRANSACTION_H
#define TRANSACTION_H
#include "utils.h"
#include <secp256k1Wrapper.h>
#include <include/web3cpp/devcore/RLP.h>
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
      uint32_t _blockIndex = 0;      // RLP + 4 BYTES Where on the block the TX is located.
      Address _from;                 // RLP + 4 BYTES + 20 BYTES (byte string)
      bool _callsContract = false;   // RLP + 4 BYTES + 20 BYTES + 1 BYTE
                                     // TOTAL: 25 Bytes.
      // _blockIndex and _inBlock is setted on State::processNewBlock and State::processNewTransaction.
      // Not stored in disk (only used for Tx Creation)
      bool _hasSig = false;
      // will only be considered _inBlock is block is confirmed.
      bool _inBlock = false;
      bool _verified = false;
    public:
      // There are two ways transactions can be parsed fully from a byte string:
      // Directly from RLP (Ethereum rawTransaction), which requires to run secp256k1 to check validity and derive _from. and it is not included in a block
      // From database (RLP bytes + Outside RLP section), input from database is trusted as data will be only saved there if included in a block and is already checked.
      // !!! BYTES IS CHANGED IF COMES FROM DB. !!!
      Base(std::string &bytes, bool fromDB);
    

      // You can also build your own Tx by inputting the values within the RLP Skeleton
      Base(Address &from, Address &to, uint256_t &value, std::string &data, uint64_t &chainId, uint256_t &nonce, uint256_t &gas, uint256_t &gasPrice) :
        _from(from), _to(to), _value(value), _data(data), _chainId(chainId), _nonce(nonce), _gas(gas), _gasPrice(gasPrice) { }
      
      // You can also create a empty transaction.
      Base() {};
      
      // Getters
      Address& to()          const { return const_cast<Address&>(_to); };
      uint256_t& value()     const { return const_cast<uint256_t&>(_value); };
      std::string& data()    const { return const_cast<std::string&>(_data); };
      uint64_t& chainId()    const { return const_cast<uint64_t&>(_chainId); };
      uint256_t& nonce()     const { return const_cast<uint256_t&>(_nonce); };
      uint256_t& gas()       const { return const_cast<uint256_t&>(_gas); };
      uint256_t& gasPrice()  const { return const_cast<uint256_t&>(_gasPrice); };
      uint256_t& v()         const { return const_cast<uint256_t&>(_v); };
      uint256_t& r()         const { return const_cast<uint256_t&>(_r); };
      uint256_t& s()         const { return const_cast<uint256_t&>(_s); };
      uint256_t recoverId()  const { return uint256_t(uint8_t(this->_v - (uint256_t(this->_chainId) * 2 + 35))); };
      uint32_t& blockIndex() const { return const_cast<uint32_t&>(_blockIndex); };
      Address& from()        const { return const_cast<Address&>(_from); };
      bool& callsContract()  const { return const_cast<bool&>(_callsContract); };
      bool& inBlock()        const { return const_cast<bool&>(_inBlock); };
      bool& hasSig()         const { return const_cast<bool&>(_hasSig); };
      bool& verified()       const { return const_cast<bool&>(_verified); };
  
      // TODO: Setters.

      void setBlockIndex (uint64_t &blockIndex) { if(_inBlock) { throw std::runtime_error("Transaction already included in a block"); }; _blockIndex = blockIndex; _inBlock = true; };
      
      // Hash in bytes not hex!
      
      std::string hash() const { std::string ret; Utils::sha3(this->rlpSerialize(this->_hasSig), ret); return ret; };
      std::string rlpSerialize(bool includeSig) const;
      std::string serialize() const;

      // Check equality, needed by std::unordered_map
      bool operator!=(Tx::Base const& tx) const { return this->hash() != tx.hash(); };
      bool operator==(Tx::Base const& tx) const { return this->hash() == tx.hash(); };
  };
}

// Hash function for std::unordered_map
template <>
struct std::hash<Tx::Base> {
  size_t operator() (const Tx::Base& tx) const {
    return std::hash<std::string>()(tx.hash());
  }
};

#endif // TRANSACTION_H