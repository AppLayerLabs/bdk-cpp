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
    Address to;                     ///< Receiver address.
    Address from;                   ///< Sender address.
    Bytes data;               ///< Arbitrary data (e.g. for contracts).
    uint64_t chainId;               ///< Chain ID where the tx will be broadcast.
    uint256_t nonce;                ///< Sender address nonce.
    uint256_t value;                ///< Value, in Wei.
    uint256_t maxPriorityFeePerGas; ///< Max priority fee per gas, as per [EIP-1559](https://eips.ethereum.org/EIPS/eip-1559), in Wei.
    uint256_t maxFeePerGas;         ///< Max fee per gas, as per [EIP-1559](https://eips.ethereum.org/EIPS/eip-1559), in Wei.
    uint256_t gasLimit;             ///< Gas limit.
    void* accessList = nullptr;     ///< Access list (not implemented).
    uint8_t v;                      ///< ECDSA recovery ID.
    uint256_t r;                    ///< ECDSA first half.
    uint256_t s;                    ///< ECDSA second half.

  public:
    /**
     * Raw constructor. Throws on parsing failure.
     * @param bytes The raw tx bytes to parse.
     * @param requiredChainId The chain ID of the transaction.
     */
    TxBlock(const BytesArrView bytes, const uint64_t& requiredChainId);

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
     * @param maxPriorityFeePerGas The maximum priority fee per gas of the transaction.
     * @param maxFeePerGas The maximum fee per gas of the transaction.
     * @param gasLimit The gas limit of the transaction.
     * @param privKey The private key used to sign the transaction.
     */
    TxBlock(
      const Address to, const Address from, const Bytes& data,
      const uint64_t chainId, const uint256_t nonce, const uint256_t value,
      const uint256_t maxPriorityFeePerGas, const uint256_t maxFeePerGas,
      const uint256_t gasLimit, const PrivKey privKey
    );

    /// Copy constructor.
    TxBlock(const TxBlock& other) noexcept :
      to(other.to),
      from(other.from),
      data(other.data),
      chainId(other.chainId),
      nonce(other.nonce),
      value(other.value),
      maxPriorityFeePerGas(other.maxPriorityFeePerGas),
      maxFeePerGas(other.maxFeePerGas),
      gasLimit(other.gasLimit),
      v(other.v),
      r(other.r),
      s(other.s)
    {}

    /// Move constructor.
    TxBlock(TxBlock&& other) noexcept :
      to(std::move(other.to)),
      from(std::move(other.from)),
      data(std::move(other.data)),
      chainId(std::move(other.chainId)),
      nonce(std::move(other.nonce)),
      value(std::move(other.value)),
      maxPriorityFeePerGas(std::move(other.maxPriorityFeePerGas)),
      maxFeePerGas(std::move(other.maxFeePerGas)),
      gasLimit(std::move(other.gasLimit)),
      v(std::move(other.v)),
      r(std::move(other.r)),
      s(std::move(other.s))
    {}

    /// Getter for `to`.
    inline const Address& getTo() const { return this->to; }

    /// Getter for `from`.
    inline const Address& getFrom() const { return this->from; }

    /// Getter for `data`.
    inline const Bytes& getData() const { return this->data; }

    /// Getter for `chainId`.
    inline const uint64_t& getChainId() const { return this->chainId; }

    /// Getter for `nonce`.
    inline const uint256_t& getNonce() const { return this->nonce; }

    /// Getter for `value`.
    inline const uint256_t& getValue() const { return this->value; }

    /// Getter for `maxPriorityFeePerGas`.
    inline const uint256_t& getMaxPriorityFeePerGas() const { return this->maxPriorityFeePerGas; }

    /// Getter for `maxPerGas`.
    inline const uint256_t& getMaxFeePerGas() const { return this->maxFeePerGas; }

    /// Getter for `gasLimit`.
    inline const uint256_t& getGasLimit() const { return this->gasLimit; }

    /// Getter for `v`.
    inline const uint8_t& getV() const { return this->v; }

    /// Getter for `r`.
    inline const uint256_t& getR() const { return this->r; }

    /// Getter for `s`.
    inline const uint256_t& getS() const { return this->s; }

    /// Getter for `v`, but calculates the real ID value based on chainId.
    inline const uint256_t recoverId() const {
      return uint256_t(uint8_t(this->v - (uint256_t(this->chainId) * 2 + 35)));
    }

    /**
     * Create a SHA3 hash of the transaction. Calls `rlpSerialize()` before hashing.
     * @param includeSig (optional) If `true`, includes the transaction signature (v/r/s).
     *                   Defaults to `true`.
     * @return The hash of the transaction, in bytes.
     */
    inline const Hash hash(bool includeSig = true) const {
      return Utils::sha3(this->rlpSerialize(includeSig));
    }

    /**
     * Serialize the transaction to a string in RLP format
     * ([EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible).
     * @param includeSig (optional) If `true`, includes the transaction signature (v/r/s).
     *                   Defaults to `true`.
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
      this->to = other.to;
      this->from = other.from;
      this->data = other.data;
      this->chainId = other.chainId;
      this->nonce = other.nonce;
      this->value = other.value;
      this->maxPriorityFeePerGas = other.maxPriorityFeePerGas;
      this->maxFeePerGas = other.maxFeePerGas;
      this->gasLimit = other.gasLimit;
      this->v = other.v;
      this->r = other.r;
      this->s = other.s;
      return *this;
    }

    /// Move assignment operator.
    TxBlock& operator=(TxBlock&& other) {
      this->to = std::move(other.to);
      this->from = std::move(other.from);
      this->data = std::move(other.data);
      this->chainId = std::move(other.chainId);
      this->nonce = std::move(other.nonce);
      this->value = std::move(other.value);
      this->maxPriorityFeePerGas = std::move(other.maxPriorityFeePerGas);
      this->maxFeePerGas = std::move(other.maxFeePerGas);
      this->gasLimit = std::move(other.gasLimit);
      this->v = std::move(other.v);
      this->r = std::move(other.r);
      this->s = std::move(other.s);
      return *this;
    }

    /// Equality operator. Checks if both transaction hashes are equal.
    bool operator==(const TxBlock& tx) const { return this->hash() == tx.hash(); }

    /// Inequality operator. Checks if both transaction hashes are different.
    bool operator!=(const TxBlock& tx) const { return this->hash() != tx.hash(); }
};

/**
 * Abstraction of a Validator transaction.
 * All transactions are final, they're defined as such during construction.
 */
class TxValidator {
  private:
    Address from;       ///< Sender address.
    Bytes data;   ///< Arbitrary data (e.g. for contracts).
    uint64_t chainId;   ///< Chain ID where the tx will be broadcast.
    uint64_t nHeight;   ///< %Block height where the tx will be broadcast.
    uint256_t v;        ///< ECDSA recovery ID.
    uint256_t r;        ///< ECDSA first half.
    uint256_t s;        ///< ECDSA second half.

  public:
    /**
     * Raw constructor.
     * Throws on parsing failure.
     * @param bytes The raw tx bytes to parse.
     * @param requiredChainId The chain ID of the transaction.
     */
    TxValidator(const BytesArrView bytes, const uint64_t& requiredChainId);

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
      const Address from, const Bytes& data, const uint64_t chainId,
      const uint64_t nHeight, const PrivKey privKey
    );

    /// Copy constructor.
    TxValidator(const TxValidator& other) noexcept :
      from(other.from),
      data(other.data),
      chainId(other.chainId),
      nHeight(other.nHeight),
      v(other.v),
      r(other.r),
      s(other.s)
    {}

    /// Move constructor.
    TxValidator(TxValidator&& other) noexcept :
      from(std::move(other.from)),
      data(std::move(other.data)),
      chainId(std::move(other.chainId)),
      nHeight(std::move(other.nHeight)),
      v(std::move(other.v)),
      r(std::move(other.r)),
      s(std::move(other.s))
    {}

    /// Getter for `from`.
    inline const Address& getFrom() const { return this->from; }

    /// Getter for `data`.
    inline const Bytes& getData() const { return this->data; }

    /// Getter for 'functor' within Data
    inline const Functor getFunctor() const { return Functor(Bytes(this->data.begin(), this->data.begin() + 4)); }

    /// Getter for `chainId`.
    inline const uint64_t& getChainId() const { return this->chainId; }

    /// Getter for `nHeight`.
    inline const uint64_t& getNHeight() const { return this->nHeight; }

    /// Getter for `v`.
    inline const uint256_t& getV() const { return this->v; }

    /// Getter for `r`.
    inline const uint256_t& getR() const { return this->r; }

    /// Getter for `s`.
    inline const uint256_t& getS() const { return this->s; }

    /// Getter for `v`, but calculates the real ID value based on chainId.
    inline const uint256_t recoverId() const {
      return uint256_t(uint8_t(this->v - (uint256_t(this->chainId) * 2 + 35)));
    }

    /**
     * Create a SHA3 hash of the transaction. Calls `rlpSerialize()` before hashing.
     * @param includeSig (optional) If `true`, includes the transaction signature (v/r/s).
     *                   Defaults to `true`.
     * @return The hash of the transaction, in bytes.
     */
    inline const Hash hash(bool includeSig = true) const {
      return Utils::sha3(this->rlpSerialize(includeSig));
    }

    /**
     * Serialize the transaction to a string in RLP format
     * ([EIP-155](https://eips.ethereum.org/EIPS/eip-155) compatible).
     * @param includeSig (optional) If `true`, includes the transaction signature (v/r/s).
     *                   Defaults to `true`.
     * @return The serialized transaction string.
     */
    Bytes rlpSerialize(bool includeSig = true) const;

    /// Copy assignment operator.
    TxValidator& operator=(const TxValidator& other) {
      this->from = other.from;
      this->data = other.data;
      this->chainId = other.chainId;
      this->nHeight = other.nHeight;
      this->v = other.v;
      this->r = other.r;
      this->s = other.s;
      return *this;
    }

    /// Move assignment operator.
    TxValidator& operator=(TxValidator&& other) {
      this->from = std::move(other.from);
      this->data = std::move(other.data);
      this->chainId = std::move(other.chainId);
      this->nHeight = std::move(other.nHeight);
      this->v = std::move(other.v);
      this->r = std::move(other.r);
      this->s = std::move(other.s);
      return *this;
    }

    /// Equality operator. Checks if both transaction hashes are equal.
    bool operator==(const TxValidator& tx) const { return this->hash() == tx.hash(); }

    /// Inequality operator. Checks if both transaction hashes are different.
    bool operator!=(const TxValidator& tx) const { return this->hash() != tx.hash(); }
};

#endif  // TX_H
