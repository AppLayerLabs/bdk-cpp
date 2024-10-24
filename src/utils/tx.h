/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef TX_H
#define TX_H

#include "ecdsa.h" // utils.h -> strings.h, (bytes/join.h -> bytes/view.h)

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
    void parseChainId(bytes::View txData, uint64_t& index);
    void parseNonce(bytes::View txData, uint64_t& index);
    void parseMaxPriorityFeePerGas(bytes::View txData, uint64_t& index);
    void parseMaxFeePerGas(bytes::View txData, uint64_t& index);
    void parseGasLimit(bytes::View txData, uint64_t& index);
    void parseTo(bytes::View txData, uint64_t& index);
    void parseValue(bytes::View txData, uint64_t& index);
    void parseData(bytes::View txData, uint64_t& index);
    void parseAccessList(bytes::View txData, uint64_t& index) const; // We don't support access lists, therefore we don't alter the object
    void parseVRS(bytes::View txData, uint64_t& index);
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
    void serializeTo(Bytes& ret) const;
    void serializeValue(Bytes& ret, const uint64_t& reqBytes) const;
    void serializeData(Bytes& ret, const uint64_t& reqBytes) const;
    void serializeVRS(Bytes& ret, const uint64_t& reqBytesR, const uint64_t& reqBytesS) const;
    ///@}

  public:
    /**
     * Raw constructor.
     * @param bytes The raw tx bytes to parse.
     * @param requiredChainId The chain ID of the transaction.
     * @throw DynamicException on any parsing failure.
     */
    TxBlock(const bytes::View bytes, const uint64_t& requiredChainId);

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

struct TxAdditionalData {
  Hash hash;
  uint64_t gasUsed;
  bool succeeded;
  Address contractAddress;
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
    Hash hash_;        ///< Transaction hash (with signature).

    ///@{
    /**
     * Parse the specified tx element from the raw byte data string starting from a given index.
     * Used exclusively by the constructor.
     * @param bytes The raw data to parse.
     * @param index The index to start parsing.
     */
    void parseData(bytes::View bytes, uint64_t& index);
    void parseNHeight(bytes::View bytes, uint64_t& index);
    void parseVRS(bytes::View bytes, uint64_t& index);
    ///@}

    ///@{
    /**
     * Serialize the specified tx element to a raw byte string.
     * Used exclusively by rlpSerialize().
     * @param ret The raw byte string to serialize to.
     * @param reqBytes The required number of bytes for the element.
     */
    void serializeData(Bytes& ret, const uint64_t& reqBytes) const;
    void serializeNHeight(Bytes& ret, const uint64_t& reqBytes) const;
    void serializeVRS(Bytes& ret, const uint64_t& reqBytesV, const uint64_t& reqBytesR, const uint64_t& reqBytesS) const;
    void serializeChainId(Bytes& ret, const uint64_t& reqBytes) const;
    ///@}

  public:
    /**
     * Raw constructor.
     * @param bytes The raw tx bytes to parse.
     * @param requiredChainId The chain ID of the transaction.
     * @throw DynamicException on any parsing failure.
     */
    TxValidator(const bytes::View bytes, const uint64_t& requiredChainId);

    /**
     * Manual constructor. Leave fields blank ("" or 0) if they're not required.
     * @param from The sender address.
     * @param data The arbitrary data string.
     * @param chainId The chain ID of the transaction.
     * @param nHeight The block height of the transaction.
     * @param privKey The private key used to sign the transaction.
     * @throw DynamicException on signing failure or sender mismatch.
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

    ///@{
    /** Getter. */
    inline const Address& getFrom() const { return this->from_; }
    inline const Bytes& getData() const { return this->data_; }
    inline Functor getFunctor() const {
      Functor ret; if (this->data_.size() < 4) return ret;
      ret.value = Utils::bytesToUint32(Utils::create_view_span(this->data_, 0, 4));
      return ret;
    }
    inline const uint64_t& getChainId() const { return this->chainId_; }
    inline const uint64_t& getNHeight() const { return this->nHeight_; }
    inline const uint256_t& getV() const { return this->v_; }
    inline const uint256_t& getR() const { return this->r_; }
    inline const uint256_t& getS() const { return this->s_; }
    inline const Hash& hash() const { return this->hash_; }
    ///@}

    /// Getter for the recovery ID, but calculates the real ID value based on chainId.
    inline uint256_t recoverId() const {
      return uint256_t(uint8_t(this->v_ - (uint256_t(this->chainId_) * 2 + 35)));
    }

    /**
     * Calculates the size of the transaction in bytes.
     * Uses the same methods as rlpSerialize() to calculate the size.
     * But without actually serializing the transaction.
     * Does NOT have a option without signature, as the signature is always included in the serialized size.
     * Serialization without the signature only happens **internally**
     * @return The size of the serialized transaction.
     */
    uint64_t rlpSize() const;

    /**
     * Serialize the transaction to a string in RLP format. [EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible.
     * @param includeSig (optional) If `true`, includes the transaction signature (v/r/s). Defaults to `true`.
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
};

#endif // TX_H
