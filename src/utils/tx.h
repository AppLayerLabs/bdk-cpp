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
    Address to_;         ///< Receiver address.
    Address from_;       ///< Sender address.
    std::string data_;   ///< Arbitrary data (e.g. for contracts).
    uint64_t chainId_;   ///< Chain ID where the tx will be broadcast.
    uint256_t nonce_;    ///< Sender address nonce.
    uint256_t value_;    ///< Value in Wei.
    uint256_t gas_;      ///< Gas limit in Wei (e.g. 21000 Wei).
    uint256_t gasPrice_; ///< Gas price in Wei (usually in Gwei - 1 Gwei = 1000000000 Wei).
    uint256_t v_;        ///< ECDSA recovery ID.
    uint256_t r_;        ///< ECDSA first half.
    uint256_t s_;        ///< ECDSA second half.
  public:
    /**
     * Raw constructor.
     * Throws on parsing failure.
     * @param bytes The raw tx bytes to parse.
     */
    TxBlock(const std::string_view& bytes);

    /**
     * Manual constructor.
     * Leave fields blank ("" or 0) if they're not required.
     * Throws on signing failure.
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
      const Address to, const Address from, const std::string data,
      const uint64_t chainId, const uint256_t nonce, const uint256_t value,
      const uint256_t gas, const uint256_t gasPrice, const PrivKey privKey
    );

    /// Copy constructor.
    TxBlock(const TxBlock& other) noexcept :
      to_(other.to_),
      from_(other.from_),
      data_(other.data_),
      chainId_(other.chainId_),
      nonce_(other.nonce_),
      value_(other.value_),
      gas_(other.gas_),
      gasPrice_(other.gasPrice_),
      v_(other.v_),
      r_(other.r_),
      s_(other.s_)
    {}

    /// Move constructor.
    TxBlock(TxBlock&& other) noexcept :
      to_(std::move(other.to_)),
      from_(std::move(other.from_)),
      data_(std::move(other.data_)),
      chainId_(std::move(other.chainId_)),
      nonce_(std::move(other.nonce_)),
      value_(std::move(other.value_)),
      gas_(std::move(other.gas_)),
      gasPrice_(std::move(other.gasPrice_)),
      v_(std::move(other.v_)),
      r_(std::move(other.r_)),
      s_(std::move(other.s_))
    {}

    /// Getter for `to`.
    inline const Address& to() const { return this->to_; }

    /// Getter for `from`.
    inline const Address& from() const { return this->from_; }

    /// Getter for `data`.
    inline const std::string& data() const { return this->data_; }

    /// Getter for `chainId`.
    inline const uint64_t& chainId() const { return this->chainId_; }

    /// Getter for `nonce`.
    inline const uint256_t& nonce() const { return this->nonce_; }

    /// Getter for `value`.
    inline const uint256_t& value() const { return this->value_; }

    /// Getter for `gas`.
    inline const uint256_t& gas() const { return this->gas_; }

    /// Getter for `gasPrice`.
    inline const uint256_t& gasPrice() const { return this->gasPrice_; }

    /// Getter for `v`.
    inline const uint256_t& v() const { return this->v_; }

    /// Getter for `r`.
    inline const uint256_t& r() const { return this->r_; }

    /// Getter for `s`.
    inline const uint256_t& s() const { return this->s_; }

    /// Getter for `v`, but calculates the real ID value based on chainId.
    inline const uint256_t recoverId() const {
      return uint256_t(uint8_t(this->v_ - (uint256_t(this->chainId_) * 2 + 35)));
    }

    /**
     * Create a SHA3 hash of the transaction. Calls `rlpSerialize()` before hashing.
     * @param (optional) includeSig If `true`, includes the transaction signature (v/r/s).
     *                   Defaults to `true`.
     * @return The hash of the transaction, in bytes.
     */
    inline const Hash hash(bool includeSig = true) const {
      return Utils::sha3(this->rlpSerialize(includeSig));
    }

    /**
     * Serialize the transaction to a string in RLP format.
     * [EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible.
     * @param (optional) includeSig If `true`, includes the transaction signature (v/r/s).
     *                   Defaults to `true`.
     * @param (optional) includeFrom If `true`, includes the sender address (for DB operations).
     *                   Defaults to `false`.
     * @return The serialized transaction.
     */
    std::string rlpSerialize(bool includeSig = true) const;

    /// Copy assignment operator.
    TxBlock& operator=(const TxBlock& other) {
      this->to_ = other.to_;
      this->from_ = other.from_;
      this->data_ = other.data_;
      this->chainId_ = other.chainId_;
      this->nonce_ = other.nonce_;
      this->value_ = other.value_;
      this->gas_ = other.gas_;
      this->gasPrice_ = other.gasPrice_;
      this->v_ = other.v_;
      this->r_ = other.r_;
      this->s_ = other.s_;
      return *this;
    }

    /// Move assignment operator.
    TxBlock& operator=(TxBlock&& other) {
      this->to_ = std::move(other.to_);
      this->from_ = std::move(other.from_);
      this->data_ = std::move(other.data_);
      this->chainId_ = std::move(other.chainId_);
      this->nonce_ = std::move(other.nonce_);
      this->value_ = std::move(other.value_);
      this->gas_ = std::move(other.gas_);
      this->gasPrice_ = std::move(other.gasPrice_);
      this->v_ = std::move(other.v_);
      this->r_ = std::move(other.r_);
      this->s_ = std::move(other.s_);
      return *this;
    }

    /// Equality operator. Checks the transaction hash.
    bool operator==(const TxBlock& tx) const { return this->hash() == tx.hash(); }

    /// Inequality operator. Checks the transaction hash.
    bool operator!=(const TxBlock& tx) const { return this->hash() != tx.hash(); }
};

/**
 * Abstraction of a Validator transaction.
 * All transactions are final, they're defined as such during construction.
 */
class TxValidator {
  private:
    Address from_;       ///< Sender address.
    std::string data_;   ///< Arbitrary data (e.g. for contracts).
    uint64_t chainId_;   ///< Chain ID where the tx will be broadcast.
    uint64_t nHeight_;   ///< Block height where the tx will be broadcast.
    uint256_t v_;        ///< ECDSA recovery ID.
    uint256_t r_;        ///< ECDSA first half.
    uint256_t s_;        ///< ECDSA second half.
  public:
    /**
     * Raw constructor.
     * Throws on parsing failure.
     * @param bytes The raw tx bytes to parse.
     */
    TxValidator(const std::string_view& bytes);

    /**
     * Manual constructor.
     * Leave fields blank ("" or 0) if they're not required.
     * Throws on signing failure.
     * @param from The sender address.
     * @param data The arbitrary data string.
     * @param chainId The chain ID of the transaction.
     * @param nHeight The block height of the transaction.
     * @param privKey The private key used to sign the transaction.
     */
    TxValidator(
      const Address from, const std::string data, const uint64_t chainId,
      const uint64_t nHeight, const PrivKey privKey
    );

    /// Copy constructor.
    TxValidator(const TxValidator& other) noexcept :
      from_(other.from_),
      data_(other.data_),
      chainId_(other.chainId_),
      nHeight_(other.nHeight_),
      v_(other.v_),
      r_(other.r_),
      s_(other.s_)
    {}

    /// Move constructor.
    TxValidator(TxValidator&& other) noexcept :
      from_(std::move(other.from_)),
      data_(std::move(other.data_)),
      chainId_(std::move(other.chainId_)),
      nHeight_(std::move(other.nHeight_)),
      v_(std::move(other.v_)),
      r_(std::move(other.r_)),
      s_(std::move(other.s_))
    {}

    /// Getter for `from`.
    inline const Address& from() const { return this->from_; }

    /// Getter for `data`.
    inline const std::string_view data() const { return this->data_; }

    /// Getter for `chainId`.
    inline const uint64_t& chainId() const { return this->chainId_; }

    /// Getter for `nHeight`.
    inline const uint64_t& nHeight() const { return this->nHeight_; }

    /// Getter for `v`.
    inline const uint256_t& v() const { return this->v_; }

    /// Getter for `r`.
    inline const uint256_t& r() const { return this->r_; }

    /// Getter for `s`.
    inline const uint256_t& s() const { return this->s_; }

    /// Getter for `v`, but calculates the real ID value based on chainId.
    inline const uint256_t recoverId() const {
      return uint256_t(uint8_t(this->v_ - (uint256_t(this->chainId_) * 2 + 35)));
    }

    /**
     * Create a SHA3 hash of the transaction. Calls `rlpSerialize()` before hashing.
     * @param (optional) includeSig If `true`, includes the transaction signature (v/r/s).
     *                   Defaults to `true`.
     * @return The hash of the transaction, in bytes.
     */
    inline const Hash hash(bool includeSig = true) const {
      return Utils::sha3(this->rlpSerialize(includeSig));
    }

    /**
     * Serialize the transaction to a string in RLP format.
     * [EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible.
     * @param (optional) includeSig If `true`, includes the transaction signature (v/r/s).
     *                   Defaults to `true`.
     * @param (optional) includeFrom If `true`, includes the sender address (for DB operations).
     *                   Defaults to `false`.
     * @return The serialized transaction.
     */
    std::string rlpSerialize(bool includeSig = true) const;

    /// Copy assignment operator.
    TxValidator& operator=(const TxValidator& other) {
      this->from_ = other.from_;
      this->data_ = other.data_;
      this->chainId_ = other.chainId_;
      this->nHeight_ = other.nHeight_;
      this->v_ = other.v_;
      this->r_ = other.r_;
      this->s_ = other.s_;
      return *this;
    }

    /// Move assignment operator.
    TxValidator& operator=(TxValidator&& other) {
      this->from_ = std::move(other.from_);
      this->data_ = std::move(other.data_);
      this->chainId_ = std::move(other.chainId_);
      this->nHeight_ = std::move(other.nHeight_);
      this->v_ = std::move(other.v_);
      this->r_ = std::move(other.r_);
      this->s_ = std::move(other.s_);
      return *this;
    }

    /// Equality operator. Checks the transaction hash.
    bool operator==(const TxValidator& tx) const { return this->hash() == tx.hash(); }

    /// Inequality operator. Checks the transaction hash.
    bool operator!=(const TxValidator& tx) const { return this->hash() != tx.hash(); }
};

#endif  // TX_H
