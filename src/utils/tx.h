/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef TX_H
#define TX_H

#include "ecdsa.h" // utils.h -> strings.h, (bytes/join.h -> bytes/view.h)
#include "uintconv.h"
#include "contract/encodedmessages.h"

/**
 * Abstraction of a block transaction.
 * All transactions are final and defined as such during construction.
 * TODO: For better const correctness within Tx classes we should transform all members to const,
 * and have static methods that return a Tx object, instead of having a constructor.
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
    uint8_t v_;                       ///< ECDSA recovery ID.
    uint256_t r_;                     ///< ECDSA first half.
    uint256_t s_;                     ///< ECDSA second half.
    Hash hash_;                       ///< Transaction hash. (with signature!)
    //void* accessList_ = nullptr;      ///< Access list (not implemented).

    ///@{
    /**
     * Parse the specified tx element from the raw byte data string starting from a given index.
     * Used exclusively by the constructor.
     * @param txData The raw data to parse.
     * @param index The index to start parsing.
     */
    void parseChainId(View<Bytes> txData, uint64_t& index);
    void parseNonce(View<Bytes> txData, uint64_t& index);
    void parseMaxPriorityFeePerGas(View<Bytes> txData, uint64_t& index);
    void parseMaxFeePerGas(View<Bytes> txData, uint64_t& index);
    void parseGasLimit(View<Bytes> txData, uint64_t& index);
    void parseTo(View<Bytes> txData, uint64_t& index);
    void parseValue(View<Bytes> txData, uint64_t& index);
    void parseData(View<Bytes> txData, uint64_t& index);
    void parseAccessList(View<Bytes> txData, uint64_t& index) const; // We don't support access lists, therefore we don't alter the object
    void parseVRS(View<Bytes> txData, uint64_t& index);
    ///@}

    ///@{
    /**
     * Serialize the specified tx element to a raw byte string.
     * Used exclusively by rlpSerialize().
     * @param ret The raw byte string to serialize to.
     * @param reqBytes The required number of bytes for the element.
     */
    void serializeChainId(Bytes& ret, const uint64_t& reqBytes) const;
    void serializeNonce(Bytes& ret, const uint64_t& reqBytes) const;
    void serializeMaxPriorityFeePerGas(Bytes& ret, const uint64_t& reqBytes) const;
    void serializeMaxFeePerGas(Bytes& ret, const uint64_t& reqBytes) const;
    void serializeGasLimit(Bytes& ret, const uint64_t& reqBytes) const;
    void serializeValue(Bytes& ret, const uint64_t& reqBytes) const;
    void serializeData(Bytes& ret, const uint64_t& reqBytes) const;
    ///@}

    /**
     * Serialize the specified tx element to a raw byte string.
     * Used exclusively by rlpSerialize().
     * @param ret The raw byte string to serialize to.
     */
    void serializeTo(Bytes& ret) const;

    /**
     * Serialize the specified tx element to a raw byte string.
     * Used exclusively by rlpSerialize().
     * @param ret The raw byte string to serialize to.
     * @param reqBytesR The required number of bytes for the element.
     * @param reqBytesS The required number of bytes for the element.
     */
    void serializeVRS(Bytes& ret, const uint64_t& reqBytesR, const uint64_t& reqBytesS) const;

  public:
    /**
     * Raw constructor.
     * @param bytes The raw tx bytes to parse.
     * @param requiredChainId Expected chain ID value.
     * @param verifySig `false` if signature verificaiton should be skipped, `true` to verify.
     * @throw DynamicException on any parsing failure or expected chain ID mismatch.
     */
    TxBlock(const View<Bytes> bytes, const uint64_t& requiredChainId, bool verifySig = true);

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
     * @throw DynamicException on signing failure or sender mismatch.
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

    ///@{
    /** Getter. */
    inline const Address& getTo() const { return this->to_; }
    inline const Address& getFrom() const { return this->from_; }
    inline const Bytes& getData() const { return this->data_; }
    inline const uint64_t& getChainId() const { return this->chainId_; }
    inline const uint256_t& getNonce() const { return this->nonce_; }
    inline const uint256_t& getValue() const { return this->value_; }
    inline const uint256_t& getMaxPriorityFeePerGas() const { return this->maxPriorityFeePerGas_; }
    inline const uint256_t& getMaxFeePerGas() const { return this->maxFeePerGas_; }
    inline const uint256_t& getGasLimit() const { return this->gasLimit_; }
    inline const uint8_t& getV() const { return this->v_; }
    inline const uint256_t& getR() const { return this->r_; }
    inline const uint256_t& getS() const { return this->s_; }
    inline const Hash& hash() const { return this->hash_; }
    ///@}

    /// Getter for the recovery ID, but calculates the real ID value based on chainId.
    inline uint256_t recoverId() const { return uint256_t(uint8_t(this->v_ - (uint256_t(this->chainId_) * 2 + 35))); }

    /**
     * Calculates the size of the transaction in bytes.
     * Uses the same methods as rlpSerialize() to calculate the size, without actually serializing the transaction.
     * Does NOT have a option without signature, as the signature is always included in the serialized size.
     * Serialization without the signature only happens **internally**.
     * @return The size of the serialized transaction.
     */
    uint64_t rlpSize() const;

    /**
     * Serialize the transaction to a string in RLP format. [EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible.
     * @param includeSig (optional) If `true`, includes the transaction signature (v/r/s). Defaults to `true`.
     * @return The serialized transaction string.
     * TODO: Serialization without signatures only happens INSIDE the constructor, perhaps we should make a private method for that.
     */
    Bytes rlpSerialize(bool includeSig = true) const;

    /**
     * Convert a TxBlock to a evmc_message object.
     * @return The equivalent evmc_message object.
     */
    evmc_message txToMessage() const;

    EncodedMessageVariant toMessage(Gas& gas) const;

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
};

/// Helper struct for separating additional (not required for tracing) data from a transaction.
struct TxAdditionalData {
  Hash hash;  ///< The transaction's hash.
  uint64_t gasUsed; ///< The cumulative gas used by the transaction.
  bool succeeded; ///< Whether the transaction suceeded or not.
  Address contractAddress;  ///< The address of the contract that made the transaction.
};

#endif // TX_H
