#ifndef TX_H
#define TX_H

#include "hash.h"
#include "strings.h"

class TxData {
  protected:
    Address to = "";
    uint256_t value = 0;
    std::string data = "";
    uint64_t chainId = 0;
    uint256_t nonce = 0;
    uint256_t gas = 0;
    uint256_t gasPrice = 0;
    uint256_t v = 0;
    uint256_t r = 0;
    uint256_t s = 0;
    uint256_t nHeight = 0;
    uint32_t blockIndex = 0;
    Address from = "";
    bool callsContract = false;
  public:
    inline void setTo(const Address& to) { this->to = to; }
    inline void setValue(const uint256_t& value) { this->value = value; }
    inline void setData(const std::string& data) { this->data = data; }
    inline void setChainId(const uint64_t& chainId) { this->chainId = chainId; }
    inline void setNonce(const uint256_t& nonce) { this->nonce = nonce; }
    inline void setGas(const uint256_t& gas) { this->gas = gas; }
    inline void setGasPrice(const uint256_t& gasPrice) { this->gasPrice = gasPrice; }
    inline void setV(const uint256_t& v) { this->v = v; }
    inline void setR(const uint256_t& r) { this->r = r; }
    inline void setS(const uint256_t& s) { this->s = s; }
    inline void setNHeight(const uint256_t& nHeight) { this->nHeight = nHeight; }
    inline void setBlockIndex(const uint32_t& blockIndex) { this->blockIndex = blockIndex; }
    inline void setFrom(const Address& from) { this->from = from; }
    inline void setCallsContract(const bool& callsContract) { this->callsContract = callsContract; }
};

class Tx {
  private:
    TxData data;
  public:
    Tx(const std::string_view& bytes, bool fromDB);
    Tx(const TxData& data);
    Tx(const Tx& other);
    Tx(Tx&& other);
    inline const Address& getTo() { return this->data.to; }
    inline const uint256_t& getValue() { return this->data.value; }
    inline const std::string& getData() { return this->data.data; }
    inline const uint64_t& getChainId() { return this->data.chainId; }
    inline const uint256_t& getNonce() { return this->data.nonce; }
    inline const uint256_t& getGas() { return this->data.gas; }
    inline const uint256_t& getGasPrice() { return this->data.gasPrice; }
    inline const uint256_t& getV() { return this->data.v; }
    inline const uint256_t& getR() { return this->data.r; }
    inline const uint256_t& getS() { return this->data.s; }
    inline const uint256_t& getNHeight() { return this->data.nHeight; }
    inline const uint32_t& getBlockIndex() { return this->data.blockIndex; }
    inline const Address& getFrom() { return this->data.from; }
    inline const bool& getCallsContract() { return this->data.callsContract; }
    inline const uint256_t recoverId() {
      return uint256_t(uint8_t(this->data.v - (uint256_t(this->data.chainId) * 2 + 35)));
    }
    Hash hash();
    const std::string rlpSerialize(bool includeSig);
    const std::string serialize();
    void sign(const PrivKey& key);
    Tx& operator=(const Tx& other);
    Tx& operator=(Tx&& other);
    bool operator!=(const Tx& tx) { return this->hash() != tx.hash(); }
    bool operator==(const Tx& tx) { return this->hash() == tx.hash(); }
};

#endif  // TX_H
