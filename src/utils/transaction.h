#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "../libs/devcore/RLP.h"
#include "../utils/secp256k1Wrapper.h"
#include "../utils/utils.h"
#include <utility>


namespace Tx {
  class Base {
    private:
      // Inside RLP, TxSkeleton:
      Address _to;
      uint256_t _value = 0;
      std::string _data;
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
      Base(const std::string_view &bytes, bool fromDB);

      // You can also build your own Tx by inputting the values within the RLP Skeleton
      Base(const Address &from, 
        const Address &to, 
        const uint256_t &value, 
        const std::string &data, 
        const uint64_t &chainId, 
        const uint256_t &nonce, 
        const uint256_t &gas, 
        const uint256_t &gasPrice) :
          _from(from), 
          _to(to), 
          _value(value), 
          _data(data), 
          _chainId(chainId), 
          _nonce(nonce), 
          _gas(gas), 
          _gasPrice(gasPrice) { }

      // You can also create an empty transaction.
      Base() {}

      // Copy constructor.
      Base(const Base& other) {
        _to = other._to;
        _value = other._value;
        _data = other._data;
        _chainId = other._chainId;
        _nonce = other._nonce;
        _gas = other._gas;
        _gasPrice = other._gasPrice;
        _v = other._v;
        _r = other._r;
        _s = other._s;
        _blockIndex = other._blockIndex;
        _from = other._from;
        _callsContract = other._callsContract;
        _hasSig = other._hasSig;
        _inBlock = other._inBlock;
        _verified = other._verified;
      }

      // Move constructor.
      Base(Base&& other) noexcept :
        _to(std::move(other._to)),
        _value(std::move(other._value)),
        _data(std::move(other._data)),
        _chainId(std::move(other._chainId)),
        _nonce(std::move(other._nonce)),
        _gas(std::move(other._gas)),
        _gasPrice(std::move(other._gasPrice)),
        _v(std::move(other._v)),
        _r(std::move(other._r)),
        _s(std::move(other._s)),
        _blockIndex(std::move(other._blockIndex)),
        _from(std::move(other._from)),
        _callsContract(std::move(other._callsContract)),
        _hasSig(std::move(other._hasSig)),
        _inBlock(std::move(other._inBlock)),
        _verified(std::move(other._verified))
      {}

          // Copy assignment operator.
      Base& operator=(const Base& other) {
        this->_to = other._to;
        this->_value = other._value;
        this->_data = other._data;
        this->_chainId = other._chainId;
        this->_nonce = other._nonce;
        this->_gas = other._gas;
        this->_gasPrice = other._gasPrice;
        this->_v = other._v;
        this->_r = other._r;
        this->_s = other._s;
        this->_blockIndex = other._blockIndex;
        this->_from = other._from;
        this->_callsContract = other._callsContract;
        this->_hasSig = other._hasSig;
        this->_inBlock = other._inBlock;
        this->_verified = other._verified;
        return *this;
      }

      // Move assignment operator.
      Base& operator=(Base&& other) {
        this->_to = std::move(other._to);
        this->_value = std::move(other._value);
        this->_data = std::move(other._data);
        this->_chainId = std::move(other._chainId);
        this->_nonce = std::move(other._nonce);
        this->_gas = std::move(other._gas);
        this->_gasPrice = std::move(other._gasPrice);
        this->_v = std::move(other._v);
        this->_r = std::move(other._r);
        this->_s = std::move(other._s);
        this->_blockIndex = std::move(other._blockIndex);
        this->_from = std::move(other._from);
        this->_callsContract = std::move(other._callsContract);
        this->_hasSig = std::move(other._hasSig);
        this->_inBlock = std::move(other._inBlock);
        this->_verified = std::move(other._verified);;
        return *this;
      }

      // Getters
      const Address& to()          const { return _to; };
      const uint256_t& value()     const { return _value; };
      const std::string& data()    const { return _data; };
      const uint64_t& chainId()    const { return _chainId; };
      const uint256_t& nonce()     const { return _nonce; };
      const uint256_t& gas()       const { return _gas; };
      const uint256_t& gasPrice()  const { return _gasPrice; };
      const uint256_t& v()         const { return _v; };
      const uint256_t& r()         const { return _r; };
      const uint256_t& s()         const { return _s; };
      const uint256_t recoverId()  const { return uint256_t(uint8_t(this->_v - (uint256_t(this->_chainId) * 2 + 35))); };
      const uint32_t& blockIndex() const { return _blockIndex; };
      const Address& from()        const { return _from; };
      const bool& callsContract()  const { return _callsContract; };
      const bool& inBlock()        const { return _inBlock; };
      const bool& hasSig()         const { return _hasSig; };
      const bool& verified()       const { return _verified; };

      // Setters
      void setTo(const Address& to) { this->_to = to; }
      void setValue(const uint256_t& value) { this->_value = value; }
      void setData(const std::string& data) { this->_data = data; }
      void setChainId(const uint64_t& chainId) { this->_chainId = chainId; }
      void setNonce(const uint256_t& nonce) { this->_nonce = nonce; }
      void setGas(const uint256_t& gas) { this->_gas = gas; }
      void setGasPrice(const uint256_t &gasPrice) { this->_gasPrice = gasPrice; }
      void setV(const uint256_t& v) { this->_v = v; }
      void setR(const uint256_t& r) { this->_r = r; }
      void setS(const uint256_t& s) { this->_s = s; }
      void setBlockIndex (const uint64_t& blockIndex) {
        if (_inBlock) throw std::runtime_error(std::string(__func__) + ": " +
          std::string("Transaction already included in a block")
        );
        this->_blockIndex = blockIndex;
        this->_inBlock = true;
      };
      void setFrom(const Address& from) { this->_from = from; }

      // Hash in bytes not hex!
      inline Hash hash() const { return Utils::sha3(this->rlpSerialize(this->_hasSig)); };
      std::string rlpSerialize(const bool &includeSig) const;
      std::string serialize() const;

      // Signer
      void sign(const PrivKey &privKey);

      // Check equality, needed by std::unordered_map
      bool operator!=(Tx::Base const& tx) const { return this->hash() != tx.hash(); };
      bool operator==(Tx::Base const& tx) const { return this->hash() == tx.hash(); };
  };

  // Validator owned transaction, there is no "to", neither gas limit, nonce, gas price or value
  // The reason for no "Nonce", is that the validator is not owner of a given account, besides that, there is a nHeight which limits
  // which block the transaction is valid in.
  // Also, there is no "blockIndex" neither other similarities.
  // The reason I didn't implement blockIndex is to speed up development, because after the implementation of the blockManager class
  // we should start doing a massive refactor.
  class Validator {
    private:
      // Inside RLP, TxSkeleton:
      std::string _data;
      uint64_t _chainId = 0;
      uint256_t _nHeight = 0;

      // Secp256k1 in RLP
      uint256_t _v = 0;
      uint256_t _r = 0;
      uint256_t _s = 0;

      // Outside RLP
      Address _from;                 // Derived from constructor/secp256k1 calculation.
      bool _hasSig = false;          

    public:
      // We *always* parse Validators signatures.
      Validator(const std::string_view &bytes);

      // You can also build your own Tx by inputting the values within the RLP Skeleton
      Validator(const Address &from,
        const std::string &data, 
        const uint64_t &chainId, 
        const uint256_t &nHeight) :
          _from(from), 
          _data(data), 
          _chainId(chainId), 
          _nHeight(nHeight) { std::cout << "a" << std::endl; }

      // Empty tx.

      Validator() {};

      // Copy constructor.
      Validator(const Validator& other) {
        this->_from = other._from;
        this->_data = other._data;
        this->_chainId = other._chainId;
        this->_nHeight = other._nHeight;
        this->_hasSig = other._hasSig;
        this->_v = other._v;
        this->_r = other._r;
        this->_s = other._s;
      }

      // Move constructor.
      Validator(Validator&& other) noexcept :
        _from(std::move(other._from)),
        _data(std::move(other._data)),
        _chainId(std::move(other._chainId)),
        _nHeight(std::move(other._nHeight)),
        _hasSig(std::move(other._hasSig)),
        _v(std::move(other._v)),
        _r(std::move(other._r)),
        _s(std::move(other._s)) {}

      // Copy assignment operator.
      Validator& operator=(const Validator& other) {
        this->_from = other._from;
        this->_data = other._data;
        this->_chainId = other._chainId;
        this->_nHeight = other._nHeight;
        this->_hasSig = other._hasSig;
        this->_v = other._v;
        this->_r = other._r;
        this->_s = other._s;
        return *this;
      }

      // Move assignment operator.
      Validator& operator=(Validator&& other) {
        this->_from = std::move(other._from);
        this->_data = std::move(other._data);
        this->_chainId = std::move(other._chainId);
        this->_nHeight = std::move(other._nHeight);
        this->_hasSig = std::move(other._hasSig);
        this->_v = std::move(other._v);
        this->_r = std::move(other._r);
        this->_s = std::move(other._s);
        return *this;
      }

      // Getters
      const std::string& data()    const { return _data; };
      const uint64_t& chainId()    const { return _chainId; };
      const uint256_t& v()         const { return _v; };
      const uint256_t& r()         const { return _r; };
      const uint256_t& s()         const { return _s; };
      const uint256_t& nHeight()   const { return _nHeight; };
      const uint256_t recoverId()  const { return uint256_t(uint8_t(this->_v - (uint256_t(this->_chainId) * 2 + 35))); };
      const Address&  from()        const { return _from; };

      // Hash in bytes not hex!
      inline Hash hash() const { return Utils::sha3(this->rlpSerialize(this->_hasSig)); };
      std::string rlpSerialize(const bool &includeSig) const;

      // Signer
      void sign(const PrivKey &privKey);

      // Check equality, needed by std::unordered_map
      bool operator!=(Tx::Validator const& tx) const { return this->hash() != tx.hash(); };
      bool operator==(Tx::Validator const& tx) const { return this->hash() == tx.hash(); };
  };
};

#endif // TRANSACTION_H
