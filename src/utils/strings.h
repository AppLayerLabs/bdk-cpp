/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STRINGS_H
#define STRINGS_H

#include <string>
#include <openssl/rand.h>
#include <span>
#include <algorithm>

#include <evmc/evmc.hpp>
#include "hex.h"

// TODO: It is possible to implement **fast** operators for some types,
// such as Address, Functor and Hash. Taking advantage that memory located within
// the array are contiguous, we can cast the data to a pointer of native types
// (such as uint64_t*) and compare them faster than using a for loop. Example:
// // Fast equality operator for h256.
// template<> inline bool FixedHash<32>::operator==(FixedHash<32> const& _other) const
// {
//   const uint64_t* hash1 = (const uint64_t*)data();
//   const uint64_t* hash2 = (const uint64_t*)_other.data();
//   return (hash1[0] == hash2[0]) && (hash1[1] == hash2[1]) && (hash1[2] == hash2[2]) && (hash1[3] == hash2[3]);
// }

/**
 * Abstraction of a fixed-size bytes container.
 * `FixedBytes<10>` would have *exactly* 10 bytes, no more, no less.
 * Used as a base for both aliases (e.g. PrivKey, PubKey, etc.) and classes inheriting it (e.g. Hash, Signature, etc.).
 */
template <unsigned N> class FixedBytes {
  protected:
    BytesArr<N> data_; ///< Internal data string, in raw bytes.

  public:
    /// Empty constructor.
    constexpr inline FixedBytes() { this->data_.fill(uint8_t{0x00}); };

    /// Move constructor.
    constexpr inline FixedBytes(BytesArr<N>&& data) noexcept { this->data_ = std::move(data); }

    /// Copy constructor.
    constexpr inline FixedBytes(const Bytes& data) {
      if (data.size() != N) throw std::invalid_argument("Invalid size.");
      std::copy(data.begin(), data.end(), this->data_.begin());
    }

    /// Copy constructor.
    constexpr inline FixedBytes(const BytesArr<N>& data) { this->data_ = data; }

    /// Copy constructor.
    constexpr inline FixedBytes(const BytesArrView& data) {
      if (data.size() != N) throw std::invalid_argument("Invalid size.");
      std::copy(data.begin(), data.end(), this->data_.begin());
    }

    /// Copy constructor.
    constexpr inline FixedBytes(const std::string_view data) {
      if (data.size() != N) throw std::invalid_argument("Invalid size.");
      std::copy(data.begin(), data.end(), this->data_.begin());
    }

    /// Copy constructor.
    constexpr inline FixedBytes(const FixedBytes& other) {
      if (other.size() != N) throw std::invalid_argument("Invalid size.");
      this->data_ = other.data_;
    }

    /// Getter for `data_`, const version.
    inline const BytesArr<N>& get() const { return this->data_; }

    /// Getter for `data_`, non-const version.
    inline BytesArr<N>& get_non_const() const { return this->data_; }

    /// Getter for `data_`, but returns it as a const C-style string.
    inline const Byte* raw() const { return this->data_.data(); }

    /// Getter for `data_`, but returns it as a non-const C-style string.
    inline Byte* raw_non_const() { return this->data_.data(); }

    /**
     * Getter for `data_`, but returns it as a hex string.
     * @param strict If `true`, returns the value with an appended "0x" prefix.
     */
    inline Hex hex(bool strict = false) const { return Hex::fromBytes(this->view(), strict); }

    /**
     * Getter for `data_`, but returns it as a span of the data string.
     * @param pos (optional) Index to start getting chars from. Defaults to the start of the string.
     * @param len (optional) Number of chars to get. Defaults to the whole string.
     * @return A string view of the data, in bytes.
     */
    inline BytesArrView view(size_t pos = 0, size_t len = N) const {
      auto real_len = std::min(len, N - pos);
      if (pos + real_len > N) { throw std::out_of_range("len > N"); }
      return BytesArrView(this->data_.begin() + pos, this->data_.begin() + pos + real_len);
    }

    /// Create a Bytes object from the internal data string.
    inline Bytes asBytes() const { return Bytes(this->data_.begin(), this->data_.end()); }

    /// Get the data string's size.
    inline size_t size() const { return this->data_.size(); }

    /// Get an iterator pointing to the start of the data string.
    inline BytesArr<N>::const_iterator cbegin() const { return this->data_.cbegin(); }

    /// Get an iterator pointing to the end of the data string.
    inline BytesArr<N>::const_iterator cend() const { return this->data_.cend(); }

    /// Equality operator. Checks if both internal strings are the same.
    inline bool operator==(const FixedBytes& other) const { return (this->data_ == other.data_); }

    /// Lesser operator. Does a lexicographical check on both data strings.
    inline bool operator<(const FixedBytes& other) const { return (this->data_ < other.data_); }

    /// Greater-or-equal operator. Does a lexicographical check on both data strings.
    inline bool operator>=(const FixedBytes& other) const { return (this->data_ >= other.data_); }

    /// Lesser-or-equal operator. Does a lexicographical check on both data strings.
    inline bool operator<=(const FixedBytes& other) const { return (this->data_ <= other.data_); }

    /// Greater operator. Does a lexicographical check on both data strings.
    inline bool operator>(const FixedBytes& other) const { return (this->data_ > other.data_); }

    /// Copy assignment operator.
    inline FixedBytes& operator=(const FixedBytes& other) {
      if (other.size() != this->size()) { throw std::invalid_argument("Invalid size."); }
      if (&other != this) this->data_ = other.data_;
      return *this;
    }

    /// Copy assignment operator.
    inline FixedBytes& operator=(const BytesArrView& other) {
      if (other.size() != this->size()) { throw std::invalid_argument("Invalid size."); }
      std::copy(other.begin(), other.end(), this->data_.begin());
      return *this;
    }

    /// Indexing operator.
    inline const Byte& operator[](const size_t pos) const { return this->data_[pos]; }

    /**
     * Operator for checking the string's "real emptyness" (all zeroes).
     * @return `true` if string is an empty value, `false` otherwise.
     */
    explicit operator bool() const { return std::any_of(this->data_.begin(), this->data_.end(), [](uint8_t _b) { return _b != 0; }); }
};

/// Abstraction of a 32-byte hash. Inherits `FixedBytes<32>`.
class Hash : public FixedBytes<32> {
  public:
    using FixedBytes<32>::FixedBytes;
    using FixedBytes<32>::operator=;
    using FixedBytes<32>::operator>=;
    using FixedBytes<32>::operator<=;
    using FixedBytes<32>::operator>;
    using FixedBytes<32>::operator<;

    /**
     * Constructor using uint256_t.
     * @param data The unsigned 256-bit number to convert into a hash string.
     */
    Hash(const uint256_t& data);

    /**
     * Constructor using a reference to evmc::bytes32.
     * @param data The evmc::bytes32 pointer to convert into a hash string.
     */
    Hash(const evmc::bytes32& data);

    /**
     * Constructor using string_view.
     * @param sv The string view to convert into a hash string.
     */
    Hash(const std::string_view sv);

    uint256_t toUint256() const;  ///< Convert the hash string back to an unsigned 256-bit number.
    evmc::bytes32 toEvmcBytes32() const;  ///< Convert the hash string back to an evmc::bytes32 pointer.

    /// Generate a random 32-byte/256-bit hash.
    inline static Hash random() { Hash h; RAND_bytes(h.data_.data(), 32); return h; }
};

/// Abstraction of a functor (the first 4 bytes of a function's keccak hash). Inherits FixedBytes<4>.
struct Functor {
  uint32_t value = 0;
  // Operator==
  inline bool operator==(const Functor& other) const { return this->value == other.value; }
};

/// Abstraction of a 65-byte ECDSA signature. Inherits `FixedBytes<65>`.
class Signature : public FixedBytes<65> {
  public:
    using FixedBytes<65>::FixedBytes;
    uint256_t r() const;  ///< Get the first half (32 bytes) of the signature.
    uint256_t s() const;  ///< Get the second half (32 bytes) of the signature.
    uint8_t v() const;  ///< Get the recovery ID (1 byte) of the signature.
};

/// Abstraction for a single 20-byte address (e.g. "1234567890abcdef..."). Inherits `FixedBytes<20>`.
class Address : public FixedBytes<20> {
  public:
    using FixedBytes<20>::operator<;
    using FixedBytes<20>::operator<=;
    using FixedBytes<20>::operator>;
    using FixedBytes<20>::operator>=;
    using FixedBytes<20>::operator=;

    /// Empty constructor.
    inline Address() { this->data_.fill(uint8_t{0x00}); };

    /**
     * Constructor using a reference to evmc::address
     * @param data The evmc::address pointer to convert into an address.
     */
    Address(const evmc::address& data);

    /**
     * Constructor using a reference to evmc_address
     * @param data The evmc_address pointer to convert into an address.
     */
    Address(const evmc_address& data);

    /**
     * Copy constructor.
     * @param add The address itself.
     * @param inBytes If `true`, treats the input as a raw bytes string.
     * @throw DynamicException if address has wrong size or is invalid.
     */
    Address(const std::string_view add, bool inBytes);

    ///@{
    /** Copy constructor. */
    inline Address(const Address& other) { this->data_ = other.data_; }
    Address(const BytesArrView add) {
      if (add.size() != 20) throw std::invalid_argument("Invalid address size");
      std::copy(add.begin(), add.end(), this->data_.begin());
    }
    Address(const BytesArr<20>& add) { std::copy(add.begin(), add.end(), this->data_.begin()); }
    template <unsigned N> Address(const BytesArr<N>& add) {
      if (add.size() != 20) throw std::invalid_argument("Invalid address size");
      std::copy(add.begin(), add.end(), this->data_.begin());
    }
    ///@}

    /// Move constructor.
    Address(BytesArr<20>&& add) : FixedBytes<20>(std::move(add)) {}

    evmc::address toEvmcAddress() const;  ///< Convert the address string back to an evmc::address.

    /**
     * Convert the address to checksum format, as per [EIP-55](https://eips.ethereum.org/EIPS/eip-55).
     * @return A copy of the checksummed address as a Hex object.
     */
    Hex toChksum() const;

    /**
     * Check if a given address string is valid.
     * If the address has both upper *and* lowercase letters, will also check the checksum.
     * @param add The address to be checked.
     * @param inBytes If `true`, treats the input as a raw bytes string.
     * @return `true` if the address is valid, `false` otherwise.
     */
    static bool isValid(const std::string_view add, bool inBytes);

    /**
     * Check if an address string is checksummed, as per [EIP-55](https://eips.ethereum.org/EIPS/eip-55).
     * Uses `toChksum()` internally. Does not alter the original string.
     * @param add The string to check.
     * @return `true` if the address is checksummed, `false` otherwise.
     */
    static bool isChksum(const std::string_view add);

    /// Copy assignment operator.
    inline Address& operator=(const Address& other) {
      this->data_ = other.data_;
      return *this;
    }
};

/// Abstraction of a EVM Storage key (20-bytes address + 32 bytes slot key). Inherits `FixedBytes<52>`.
class StorageKey : public FixedBytes<52> {
  public:
    using FixedBytes<52>::operator<;
    using FixedBytes<52>::operator<=;
    using FixedBytes<52>::operator>;
    using FixedBytes<52>::operator>=;
    using FixedBytes<52>::operator=;

    /**
     * Constructor using a BytesArrView
     * @param data The BytesArrView pointer to convert into a storage key.
     */
    StorageKey(const BytesArrView& data) {
      if (data.size() != 52) throw std::invalid_argument("Invalid StorageKey size.");
      std::copy(data.begin(), data.end(), this->data_.begin());
    }

    /**
     * Constructor using a reference to evmc::address and a reference to evmc::bytes32.
     * @param addr The evmc::address pointer to convert into a storage key.
     * @param slot The evmc::bytes32 pointer to convert into a storage key.
     */
    StorageKey(const evmc::address& addr, const evmc::bytes32& slot);

    /**
     * Constructor using a reference to evmc_address and a reference to evmc_bytes32.
     * @param addr The evmc_address pointer to convert into a storage key.
     * @param slot The evmc::bytes32 pointer to convert into a storage key.
     */
    StorageKey(const evmc_address& addr, const evmc_bytes32& slot);

    /**
     * Constructor using a reference to evmc_address and a reference to evmc::bytes32.
     * @param addr The evmc::address pointer to convert into a storage key.
     * @param slot The evmc::bytes32 pointer to convert into a storage key.
     */
    StorageKey(const evmc_address& addr, const evmc::bytes32& slot);

     /**
      * Constructor using a reference to evmc::address and a reference to evmc_bytes32.
      * @param addr The evmc_address pointer to convert into a storage key.
      * @param slot The evmc::bytes32 pointer to convert into a storage key.
      */
    StorageKey(const evmc::address& addr, const evmc_bytes32& slot);

    /**
     * Constructor using a reference to Address and a reference to Hash.
     * @param addr The Address pointer to convert into a storage key.
     * @param slot The Hash pointer to convert into a storage key.
     */
    StorageKey(const Address& addr, const Hash& slot);
};


#endif  // STRINGS_H
