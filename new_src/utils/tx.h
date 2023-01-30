#ifndef TX_H
#define TX_H

#include "ecdsa.h"
#include "strings.h"
#include "utils.h"

/**
 * Abstraction of a block transaction.
 * All transactions are final, they're defined as such during construction.
 */
class TxBlock {
  private:
    Address to;         ///< Receiver address.
    Address from;       ///< Sender address.
    std::string data;   ///< Arbitrary data (e.g. for contracts).
    uint64_t chainId;   ///< Chain ID where the tx will be broadcast.
    uint256_t nonce;    ///< Sender address nonce.
    uint256_t value;    ///< Value in Wei.
    uint256_t gas;      ///< Gas limit in Wei (e.g. 21000 Wei).
    uint256_t gasPrice; ///< Gas price in Wei (usually in Gwei - 1 Gwei = 1000000000 Wei).
    uint256_t v;        ///< ECDSA recovery ID.
    uint256_t r;        ///< ECDSA first half.
    uint256_t s;        ///< ECDSA second half.
  public:
    /**
     * Raw constructor.
     * @param bytes The raw tx bytes to parse.
     * @param fromDB If `true`, assumes the tx comes from the database.
     *               If `false`, assumes the tx comes from RLP.
     *               DB txs are trusted, as they are already validated and
     *               included in a block, thus no checks are needed.
     *               RLP txs are untrusted, as they are not validated or
     *               included in a block yet, thus they need to be checked
     *               with secp256k1 (as well as derive the sender address).
     */
    TxBlock(const std::string_view& bytes, bool fromDB);

    /**
     * Manual constructor. Leave fields blank ("" or 0) if they're not required.
     * @param to The receiver address.
     * @param from The sender address.
     * @param data The arbitrary data string.
     * @param chainId The chain ID of the transaction.
     * @param nonce The nonce of the transaction.
     * @param value The value of the transaction.
     * @param gas The gas limit of the transaction.
     * @param gasPrice The gas price of the transaction.
     * @param privKey The private key used to sign the transaction.
     */
    TxBlock(
      Address to, Address from, std::string data, uint64_t chainId, uint256_t nonce,
      uint256_t value, uint256_t gas, uint256_t gasPrice, PrivKey privKey
    ) : to(to), from(from), data(data), chainId(chainId), nonce(nonce),
      value(value), gas(gas), gasPrice(gasPrice)
    {
      ; // TODO: derivate v/r/s using privKey
    }

    /// Copy constructor.
    TxBlock(const TxBlock& other) {
      this->to = other.to;
      this->from = other.from;
      this->data = other.data;
      this->chainId = other.chainId;
      this->nonce = other.nonce;
      this->value = other.value;
      this->gas = other.gas;
      this->gasPrice = other.gasPrice;
      this->v = other.v;
      this->r = other.r;
      this->s = other.s;
    }

    /// Move constructor.
    TxBlock(TxBlock&& other) noexcept :
      to(std::move(other.to)),
      from(std::move(other.from)),
      data(std::move(other.data)),
      chainId(std::move(other.chainId)),
      nonce(std::move(other.nonce)),
      value(std::move(other.value)),
      gas(std::move(other.gas)),
      gasPrice(std::move(other.gasPrice)),
      v(std::move(other.v)),
      r(std::move(other.r)),
      s(std::move(other.s)),
    {}

    /// Getter for `to`.
    inline const Address& getTo() { return this->to; }

    /// Getter for `from`.
    inline const Address& getFrom() { return this->from; }

    /// Getter for `data`.
    inline const std::string& getData() { return this->data; }

    /// Getter for `chainId`.
    inline const uint64_t& getChainId() { return this->chainId; }

    /// Getter for `nonce`.
    inline const uint256_t& getNonce() { return this->nonce; }

    /// Getter for `value`.
    inline const uint256_t& getValue() { return this->value; }

    /// Getter for `gas`.
    inline const uint256_t& getGas() { return this->gas; }

    /// Getter for `gasPrice`.
    inline const uint256_t& getGasPrice() { return this->gasPrice; }

    /// Getter for `v`.
    inline const uint256_t& getV() { return this->v; }

    /// Getter for `r`.
    inline const uint256_t& getR() { return this->r; }

    /// Getter for `s`.
    inline const uint256_t& getS() { return this->s; }

    /// Getter for `v`, but calculates the real ID value based on chainId.
    inline const uint256_t recoverId() {
      return uint256_t(uint8_t(this->v - (uint256_t(this->chainId) * 2 + 35)));
    }

    /**
     * Create a SHA3 hash of the transaction. Calls `rlpSerialize()` before hashing.
     * @return The hash of the transaction, in bytes.
     */
    Hash hash();

    /**
     * Serialize the transaction to a string in RLP format.
     * [EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible.
     * @param (optional) includeSig If `true`, includes the transaction signature (v/r/s).
     *                   Defaults to `true`.
     * @return The serialized transaction.
     */
    const std::string rlpSerialize(bool includeSig = true);

    /**
     * Same as `rlpSerialize()`, but checks if tx has a signature/is verified,
     * and includes extra content like block index, sender address, if the tx
     * calls a contract and if it's in a block.
     */
    const std::string serialize();

    /// Copy assignment operator.
    TxBlock& operator=(const TxBlock& other);

    /// Move assignment operator.
    TxBlock& operator=(TxBlock&& other);

    /// Equality operator. Checks the transaction hash.
    bool operator==(const TxBlock& tx) { return this->hash() == tx.hash(); }

    /// Inequality operator. Checks the transaction hash.
    bool operator!=(const TxBlock& tx) { return this->hash() != tx.hash(); }
};

/**
 * Abstraction of a Validator transaction.
 * All transactions are final, they're defined as such during construction.
 */
class TxValidator {
  private:
    Address from;       ///< Sender address.
    std::string data;   ///< Arbitrary data (e.g. for contracts).
    uint64_t chainId;   ///< Chain ID where the tx will be broadcast.
    uint64_t nHeight;   ///< Block height where the tx will be broadcast.
    uint256_t v;        ///< ECDSA recovery ID.
    uint256_t r;        ///< ECDSA first half.
    uint256_t s;        ///< ECDSA second half.
  public:
    /**
     * Raw constructor.
     * @param bytes The raw tx bytes to parse.
     * @param fromDB If `true`, assumes the tx comes from the database.
     *               If `false`, assumes the tx comes from RLP.
     *               DB txs are trusted, as they are already validated and
     *               included in a block, thus no checks are needed.
     *               RLP txs are untrusted, as they are not validated or
     *               included in a block yet, thus they need to be checked
     *               with secp256k1 (as well as derive the sender address).
     */
    TxValidator(const std::string_view& bytes, bool fromDB);

    /**
     * Manual constructor. Leave fields blank ("" or 0) if they're not required.
     * @param from The sender address.
     * @param data The arbitrary data string.
     * @param chainId The chain ID of the transaction.
     * @param nHeight The block height of the transaction.
     * @param privKey The private key used to sign the transaction.
     */
    TxValidator(
      Address from, std::string data, uint64_t chainId, uint64_t nHeight, PrivKey privKey
    ) : from(from), data(data), chainId(chainId), nHeight(nHeight) {
      ; // TODO: derivate v/r/s using privKey
    }

    /// Copy constructor.
    TxValidator(const TxValidator& other) {
      this->from = other.from;
      this->data = other.data;
      this->chainId = other.chainId;
      this->nHeight = other.nHeight;
      this->v = other.v;
      this->r = other.r;
      this->s = other.s;
    }

    /// Move constructor.
    TxValidator(TxValidator&& other) noexcept :
      from(std::move(other.from)),
      data(std::move(other.data)),
      chainId(std::move(other.chainId)),
      nHeight(std::move(other.nHeight)),
      v(std::move(other.v)),
      r(std::move(other.r)),
      s(std::move(other.s)),
    {}

    /// Getter for `from`.
    inline const Address& getFrom() { return this->from; }

    /// Getter for `data`.
    inline const std::string& getData() { return this->data; }

    /// Getter for `chainId`.
    inline const uint64_t& getChainId() { return this->chainId; }

    /// Getter for `nHeight`.
    inline const uint64_t& getNHeight() { return this->nHeight; }

    /// Getter for `v`.
    inline const uint256_t& getV() { return this->v; }

    /// Getter for `r`.
    inline const uint256_t& getR() { return this->r; }

    /// Getter for `s`.
    inline const uint256_t& getS() { return this->s; }

    /// Getter for `v`, but calculates the real ID value based on chainId.
    inline const uint256_t recoverId() {
      return uint256_t(uint8_t(this->v - (uint256_t(this->chainId) * 2 + 35)));
    }

    /**
     * Create a SHA3 hash of the transaction. Calls `rlpSerialize()` before hashing.
     * @return The hash of the transaction, in bytes.
     */
    Hash hash();

    /**
     * Serialize the transaction to a string in RLP format.
     * [EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible.
     * @param (optional) includeSig If `true`, includes the transaction signature (v/r/s).
     *                   Defaults to `true`.
     * @return The serialized transaction.
     */
    const std::string rlpSerialize(bool includeSig = true);

    /**
     * Same as `rlpSerialize()`, but checks if tx has a signature/is verified,
     * and includes extra content like block index, sender address, if the tx
     * calls a contract and if it's in a block.
     */
    const std::string serialize();

    /// Copy assignment operator.
    TxValidator& operator=(const TxValidator& other);

    /// Move assignment operator.
    TxValidator& operator=(TxValidator&& other);

    /// Equality operator. Checks the transaction hash.
    bool operator==(const TxValidator& tx) { return this->hash() == tx.hash(); }

    /// Inequality operator. Checks the transaction hash.
    bool operator!=(const TxValidator& tx) { return this->hash() != tx.hash(); }
};

#endif  // TX_H
