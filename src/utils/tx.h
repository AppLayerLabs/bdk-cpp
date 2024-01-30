/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef TX_H
#define TX_H

#include "ecdsa.h"
#include "strings.h"
#include "utils.h"

/**
 * Abstraction of a block transaction.
 * All transactions are final and defined as such during construction.
 * TODO: For better const correctness within Tx classes
 * we should transfor all members to const, and have static methods
 * That returns a Tx object, instead of having a constructor.
 */
class TxBlock {
  private:
    Address to_;                      ///< Receiver address.
    Address from_;                    ///< Sender address.
    Bytes data_;                      ///< Arbitrary data (e.g. for contracts).
    uint64_t chainId_;                ///< Chain ID where the tx will be broadcast.
    uint256_t nonce_;                 ///< Sender address nonce.
    uint256_t value_;                 ///< Value, in Wei.
    uint256_t maxPriorityFeePerGas_;  ///< Max priority fee per gas, as per [EIP-1559](https://eips.ethereum.org/EIPS/eip-1559), in Wei.
    uint256_t maxFeePerGas_;          ///< Max fee per gas, as per [EIP-1559](https://eips.ethereum.org/EIPS/eip-1559), in Wei.
    uint256_t gasLimit_;              ///< Gas limit.
    void* accessList_ = nullptr;      ///< Access list (not implemented).
    uint8_t v_;                       ///< ECDSA recovery ID.
    uint256_t r_;                     ///< ECDSA first half.
    uint256_t s_;                     ///< ECDSA second half.
    Hash hash_;                       ///< Transaction hash. (with signature!)

  public:
    /**
     * Raw constructor.
     * @param bytes The raw tx bytes to parse.
     * @param requiredChainId The chain ID of the transaction.
     * @throw std::runtime_error on any parsing failure.
     */
    TxBlock(const BytesArrView bytes, const uint64_t& requiredChainId);

    /**
     * Manual constructor. Leave fields blank ("" or 0) if they're not required.
     * @param to The receiver address.
     * @param from The sender address.
     * @param data The arbitrary data string.
     * @param chainId The chain ID of the transaction.
     * @param nonce The nonce of the transaction.
     * @param value The value of the transaction.
     * @param maxPriorityFeePerGas The maximum priority fee per gas of the transaction.
     * @param maxFeePerGas The maximum fee per gas of the transaction.
     * @param gasLimit The gas limit of the transaction.
     * @param privKey The private key used to sign the transaction.
     * @throw std::runtime_error on signing failure or sender mismatch.
     */
    TxBlock(
      const Address& to, const Address& from, const Bytes& data,
      const uint64_t& chainId, const uint256_t& nonce, const uint256_t& value,
      const uint256_t& maxPriorityFeePerGas, const uint256_t& maxFeePerGas,
      const uint256_t& gasLimit, const PrivKey& privKey
    );

    /// Copy constructor.
    TxBlock(const TxBlock& other) noexcept
    : to_(other.to_), from_(other.from_), data_(other.data_),
    chainId_(other.chainId_), nonce_(other.nonce_), value_(other.value_),
    maxPriorityFeePerGas_(other.maxPriorityFeePerGas_),
    maxFeePerGas_(other.maxFeePerGas_), gasLimit_(other.gasLimit_),
    v_(other.v_), r_(other.r_), s_(other.s_), hash_(other.hash_) {}

    /// Move constructor.
    TxBlock(TxBlock&& other) noexcept
    : to_(std::move(other.to_)), from_(std::move(other.from_)),
    data_(std::move(other.data_)), chainId_(std::move(other.chainId_)),
    nonce_(std::move(other.nonce_)), value_(std::move(other.value_)),
    maxPriorityFeePerGas_(std::move(other.maxPriorityFeePerGas_)),
    maxFeePerGas_(std::move(other.maxFeePerGas_)),
    gasLimit_(std::move(other.gasLimit_)), v_(std::move(other.v_)),
    r_(std::move(other.r_)), s_(std::move(other.s_)), hash_(std::move(other.hash_)) {}

    /// Getter for `to_`.
    inline const Address& getTo() const { return this->to_; }

    /// Getter for `from_`.
    inline const Address& getFrom() const { return this->from_; }

    /// Getter for `data_`.
    inline const Bytes& getData() const { return this->data_; }

    /// Getter for `chainId_`.
    inline const uint64_t& getChainId() const { return this->chainId_; }

    /// Getter for `nonce_`.
    inline const uint256_t& getNonce() const { return this->nonce_; }

    /// Getter for `value_`.
    inline const uint256_t& getValue() const { return this->value_; }

    /// Getter for `maxPriorityFeePerGas_`.
    inline const uint256_t& getMaxPriorityFeePerGas() const { return this->maxPriorityFeePerGas_; }

    /// Getter for `maxFeePerGas_`.
    inline const uint256_t& getMaxFeePerGas() const { return this->maxFeePerGas_; }

    /// Getter for `gasLimit_`.
    inline const uint256_t& getGasLimit() const { return this->gasLimit_; }

    /// Getter for `v_`.
    inline const uint8_t& getV() const { return this->v_; }

    /// Getter for `r_`.
    inline const uint256_t& getR() const { return this->r_; }

    /// Getter for `s_`.
    inline const uint256_t& getS() const { return this->s_; }

    /// Getter for `v_`, but calculates the real ID value based on chainId.
    inline const uint256_t recoverId() const {
      return uint256_t(uint8_t(this->v_ - (uint256_t(this->chainId_) * 2 + 35)));
    }

    /**
     * Getter for `hash_`.
     * @return The hash of the transaction, in bytes.
     */
    inline const Hash& hash() const {
      return this->hash_;
    }

    /**
     * Serialize the transaction to a string in RLP format
     * ([EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible).
     * @param includeSig (optional) If `true`, includes the transaction signature
     * (v/r/s). Defaults to `true`.
     * @return The serialized transaction string.
     */
    Bytes rlpSerialize(bool includeSig = true) const;

    /**
     * Convert a TxBlock to a ethCallInfo object
     * @param txBlock The TxBlock to convert.
     * @return The equivalent ethCallInfo object.
     */
    ethCallInfo txToCallInfo() const;

    /// Copy assignment operator.
    TxBlock& operator=(const TxBlock& other) {
      this->to_ = other.to_;
      this->from_ = other.from_;
      this->data_ = other.data_;
      this->chainId_ = other.chainId_;
      this->nonce_ = other.nonce_;
      this->value_ = other.value_;
      this->maxPriorityFeePerGas_ = other.maxPriorityFeePerGas_;
      this->maxFeePerGas_ = other.maxFeePerGas_;
      this->gasLimit_ = other.gasLimit_;
      this->v_ = other.v_;
      this->r_ = other.r_;
      this->s_ = other.s_;
      this->hash_ = other.hash_;
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
      this->maxPriorityFeePerGas_ = std::move(other.maxPriorityFeePerGas_);
      this->maxFeePerGas_ = std::move(other.maxFeePerGas_);
      this->gasLimit_ = std::move(other.gasLimit_);
      this->v_ = std::move(other.v_);
      this->r_ = std::move(other.r_);
      this->s_ = std::move(other.s_);
      this->hash_ = std::move(other.hash_);
      return *this;
    }

    /// Equality operator. Checks if both transaction hashes are equal.
    bool operator==(const TxBlock& tx) const { return this->hash() == tx.hash(); }

    /// Inequality operator. Checks if both transaction hashes are different.
    bool operator!=(const TxBlock& tx) const { return this->hash() != tx.hash(); }
};

/**
 * Abstraction of a Validator transaction.
 * All transactions are final and defined as such during construction.
 */
class TxValidator {
  private:
    Address from_;     ///< Sender address.
    Bytes data_;       ///< Arbitrary data (e.g. for contracts).
    uint64_t chainId_; ///< Chain ID where the tx will be broadcast.
    uint64_t nHeight_; ///< %Block height where the tx will be broadcast.
    uint256_t v_;      ///< ECDSA recovery ID.
    uint256_t r_;      ///< ECDSA first half.
    uint256_t s_;      ///< ECDSA second half.
    Hash hash_;                       ///< Transaction hash. (with signature!)

  public:
    /**
     * Raw constructor.
     * @param bytes The raw tx bytes to parse.
     * @param requiredChainId The chain ID of the transaction.
     * @throw std::runtime_error on any parsing failure.
     */
    TxValidator(const BytesArrView bytes, const uint64_t& requiredChainId);

    /**
     * Manual constructor. Leave fields blank ("" or 0) if they're not required.
     * @param from The sender address.
     * @param data The arbitrary data string.
     * @param chainId The chain ID of the transaction.
     * @param nHeight The block height of the transaction.
     * @param privKey The private key used to sign the transaction.
     * @throw std::runtime_error on signing failure or sender mismatch.
     */
    TxValidator(
      const Address& from, const Bytes& data, const uint64_t& chainId,
      const uint64_t& nHeight, const PrivKey& privKey
    );

    /// Copy constructor.
    TxValidator(const TxValidator& other) noexcept
    : from_(other.from_), data_(other.data_), chainId_(other.chainId_),
    nHeight_(other.nHeight_), v_(other.v_), r_(other.r_), s_(other.s_), hash_(other.hash_) {}

    /// Move constructor.
    TxValidator(TxValidator&& other) noexcept
    : from_(std::move(other.from_)), data_(std::move(other.data_)),
    chainId_(std::move(other.chainId_)), nHeight_(std::move(other.nHeight_)),
    v_(std::move(other.v_)), r_(std::move(other.r_)), s_(std::move(other.s_)), hash_(std::move(other.hash_)) {}

    /// Getter for `from`.
    inline const Address& getFrom() const { return this->from_; }

    /// Getter for `data`.
    inline const Bytes& getData() const { return this->data_; }

    /// Getter for the functor within `data`.
    inline const Functor getFunctor() const {
      return Functor(Bytes(this->data_.begin(), this->data_.begin() + 4));
    }

    /// Getter for `chainId`.
    inline const uint64_t& getChainId() const { return this->chainId_; }

    /// Getter for `nHeight`.
    inline const uint64_t& getNHeight() const { return this->nHeight_; }

    /// Getter for `v`.
    inline const uint256_t& getV() const { return this->v_; }

    /// Getter for `r`.
    inline const uint256_t& getR() const { return this->r_; }

    /// Getter for `s`.
    inline const uint256_t& getS() const { return this->s_; }

    /// Getter for `v`, but calculates the real ID value based on chainId.
    inline const uint256_t recoverId() const {
      return uint256_t(uint8_t(this->v_ - (uint256_t(this->chainId_) * 2 + 35)));
    }

    /**
     * Getter for `hash_`.
     * @return The hash of the transaction, in bytes.
     */
    inline const Hash& hash() const {
      return this->hash_;
    }

    /**
     * Serialize the transaction to a string in RLP format
     * ([EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible).
     * @param includeSig (optional) If `true`, includes the transaction signature
     * (v/r/s). Defaults to `true`.
     * @return The serialized transaction string.
     */
    Bytes rlpSerialize(bool includeSig = true) const;

    /// Copy assignment operator.
    TxValidator& operator=(const TxValidator& other) {
      this->from_ = other.from_;
      this->data_ = other.data_;
      this->chainId_ = other.chainId_;
      this->nHeight_ = other.nHeight_;
      this->v_ = other.v_;
      this->r_ = other.r_;
      this->s_ = other.s_;
      this->hash_ = other.hash_;
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
      this->hash_ = std::move(other.hash_);
      return *this;
    }

    /// Equality operator. Checks if both transaction hashes are equal.
    bool operator==(const TxValidator& tx) const { return this->hash() == tx.hash(); }

    /// Inequality operator. Checks if both transaction hashes are different.
    bool operator!=(const TxValidator& tx) const { return this->hash() != tx.hash(); }
};

#endif // TX_H
