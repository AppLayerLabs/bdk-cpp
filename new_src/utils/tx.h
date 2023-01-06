#ifndef TX_H
#define TX_H

#include "hash.h"
#include "strings.h"

/**
 * Abstraction of a base transaction.
 * Data inside this type of transaction is never final, it's meant to be
 * built like it was a struct, then "finalized" by putting it in a %Tx object.
 */
class TxData {
  protected:
    Address to = "";            ///< Receiver address.
    uint256_t value = 0;        ///< Value in Wei.
    std::string data = "";      ///< Arbitrary data (e.g. for contracts).
    uint64_t chainId = 0;       ///< Chain ID where the tx will be broadcast.
    uint256_t nonce = 0;        ///< Sender address nonce.
    uint256_t gas = 0;          ///< Gas limit in Wei (e.g. 21000 Wei).
    uint256_t gasPrice = 0;     ///< Gas price in Wei (usually in Gwei - 1 Gwei = 1000000000 Wei).
    uint256_t v = 0;            ///< ECDSA recovery ID.
    uint256_t r = 0;            ///< ECDSA first half.
    uint256_t s = 0;            ///< ECDSA second half.
    uint256_t nHeight = 0;      ///< Block height.
    uint32_t blockIndex = 0;    ///< Index of tx inside the block.
    Address from = "";          ///< Sender address.
    bool callsContract = false; ///< Indicates whether the tx calls a contract.
  public:
    /// Setter for `to`.
    inline void setTo(const Address& to) { this->to = to; }

    /// Setter for `value`.
    inline void setValue(const uint256_t& value) { this->value = value; }

    /// Setter for `data`.
    inline void setData(const std::string& data) { this->data = data; }

    /// Setter for `chainId`.
    inline void setChainId(const uint64_t& chainId) { this->chainId = chainId; }

    /// Setter for `nonce`.
    inline void setNonce(const uint256_t& nonce) { this->nonce = nonce; }

    /// Setter for `gas`.
    inline void setGas(const uint256_t& gas) { this->gas = gas; }

    /// Setter for `gasPrice`.
    inline void setGasPrice(const uint256_t& gasPrice) { this->gasPrice = gasPrice; }

    /// Setter for `v`.
    inline void setV(const uint256_t& v) { this->v = v; }

    /// Setter for `r`.
    inline void setR(const uint256_t& r) { this->r = r; }

    /// Setter for `s`.
    inline void setS(const uint256_t& s) { this->s = s; }

    /// Setter for `nHeight`.
    inline void setNHeight(const uint256_t& nHeight) { this->nHeight = nHeight; }

    /// Setter for `blockIndex`.
    inline void setBlockIndex(const uint32_t& blockIndex) { this->blockIndex = blockIndex; }

    /// Setter for `from`.
    inline void setFrom(const Address& from) { this->from = from; }

    /// Setter for `callsContract`.
    inline void setCallsContract(const bool& callsContract) { this->callsContract = callsContract; }
};

/**
 * Abstraction of a finalized transaction.
 * Data inside this type of transaction is always final and can't be altered.
 */
class Tx {
  private:
    TxData data;  ///< Finalized tx data.
  public:
    /**
     * Constructor.
     * @param bytes The raw tx bytes to parse.
     * @param fromDB If `true`, assumes the tx comes from the database.
     *               If `false`, assumes the tx comes from RLP.
     *               DB txs are trusted, as they are already validated and
     *               included in a block, thus no checks are needed.
     *               RLP txs are untrusted, as they are not validated or
     *               included in a block yet, thus they need to be checked
     *               with secp256k1 (as well as derive the sender address).
     */
    Tx(const std::string_view& bytes, bool fromDB);
    Tx(const TxData& data); ///< Copy constructor.
    Tx(const Tx& other);    ///< Copy constructor.
    Tx(Tx&& other);         ///< Move constructor.

    /// Getter for `to`.
    inline const Address& getTo() { return this->data.to; }

    /// Getter for `value`.
    inline const uint256_t& getValue() { return this->data.value; }

    /// Getter for `data`.
    inline const std::string& getData() { return this->data.data; }

    /// Getter for `chainId`.
    inline const uint64_t& getChainId() { return this->data.chainId; }

    /// Getter for `nonce`.
    inline const uint256_t& getNonce() { return this->data.nonce; }

    /// Getter for `gas`.
    inline const uint256_t& getGas() { return this->data.gas; }

    /// Getter for `gasPrice`.
    inline const uint256_t& getGasPrice() { return this->data.gasPrice; }

    /// Getter for `v`.
    inline const uint256_t& getV() { return this->data.v; }

    /// Getter for `r`.
    inline const uint256_t& getR() { return this->data.r; }

    /// Getter for `s`.
    inline const uint256_t& getS() { return this->data.s; }

    /// Getter for `nHeight`.
    inline const uint256_t& getNHeight() { return this->data.nHeight; }

    /// Getter for `blockIndex`.
    inline const uint32_t& getBlockIndex() { return this->data.blockIndex; }

    /// Getter for `from`.
    inline const Address& getFrom() { return this->data.from; }

    /// Getter for `callsContract`.
    inline const bool& getCallsContract() { return this->data.callsContract; }

    /// Getter for `v`, but calculates the real ID value based on chainId.
    inline const uint256_t recoverId() {
      return uint256_t(uint8_t(this->data.v - (uint256_t(this->data.chainId) * 2 + 35)));
    }

    /**
     * Create a SHA3 hash of the transaction. Calls `rlpSerialize()` before hashing.
     * @return The hash of the transaction, in bytes.
     */
    Hash hash();

    /**
     * Serialize the transaction to a string in RLP format.
     * [EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible.
     * @param includeSig If `true`, includes the transaction signature (v/r/s).
     * @return The serialized transaction.
     */
    const std::string rlpSerialize(bool includeSig);

    /**
     * Same as `rlpSerialize()`, but checks if tx has a signature/is verified,
     * and includes extra content like block index, sender address, if the tx
     * calls a contract and if it's in a block.
     */
    const std::string serialize();

    /**
     * Sign the transaction with a given private key.
     * @param key The private key to sign with.
     */
    void sign(const PrivKey& key);

    /// Copy assignment operator.
    Tx& operator=(const Tx& other);

    /// Move assignment operator.
    Tx& operator=(Tx&& other);

    /// Inequality operator. Checks the transaction hash.
    bool operator!=(const Tx& tx) { return this->hash() != tx.hash(); }

    /// Equality operator. Checks the transaction hash.
    bool operator==(const Tx& tx) { return this->hash() == tx.hash(); }
};

#endif  // TX_H
