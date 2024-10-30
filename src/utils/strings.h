/*
Copyright (c) [2023-2024] [AppLayer Developers]

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
#include "bytes/range.h"
#include "bytes/initializer.h"
#include "zpp_bits.h"

#include "address.h"
#include "hash.h"
#include "signature.h"

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
  private:
    BytesArr<N> data_;

    friend zpp::bits::access;
    using serialize = zpp::bits::members<1>;

  public:
    constexpr FixedBytes() : data_() {};

    constexpr FixedBytes(std::initializer_list<Byte> initList) {
      if (initList.size() != N)
        throw DynamicException("Given initializer list of size " + std::to_string(initList.size()) + 
          " is not suitable for initializing a FixedBytes<" + std::to_string(N) + ">");

      std::ranges::copy(initList, data_.begin());
    }

    constexpr FixedBytes(const bytes::Initializer auto& initializer) { initializer.to(data_); }

    constexpr explicit FixedBytes(const bytes::Range auto& data) {
      if (const size_t size = std::ranges::size(data); size != N)
        throw DynamicException("Given bytes range of size " + std::to_string(size) + 
          " is not suitable for initializing a FixedBytes<" + std::to_string(N) + ">");

      std::ranges::copy(data, data_.begin());
    }

    constexpr auto begin() { return data_.begin(); }

    constexpr auto begin() const { return data_.begin(); }

    constexpr auto cbegin() const { return data_.cbegin(); }

    constexpr auto end() { return data_.end(); }

    constexpr auto end() const { return data_.end(); }

    constexpr auto cend() const { return data_.cend(); }

    constexpr Byte* data() { return data_.data(); }

    constexpr const Byte* data() const { return data_.data(); }

    constexpr size_t size() const { return data_.size(); }

    constexpr Byte& operator[](size_t index) { return data_[index]; }

    constexpr const Byte& operator[](size_t index) const { return data_[index]; }

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
    inline bytes::View view(size_t pos = 0, size_t len = N) const {
      auto real_len = std::min(len, N - pos);
      if (pos + real_len > N) { throw std::out_of_range("len > N"); }
      return bytes::View(this->data_.begin() + pos, this->data_.begin() + pos + real_len);
    }

    /// Create a Bytes object from the internal data string.
    inline Bytes asBytes() const { return Bytes(this->data_.begin(), this->data_.end()); }

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

    /**
     * Operator for checking the string's "real emptyness" (all zeroes).
     * @return `true` if string is an empty value, `false` otherwise.
     */
    explicit operator bool() const {
      return std::ranges::any_of(*this, [] (Byte b) { return b != 0; });
    }
};

/// Abstraction of a functor (the first 4 bytes of a function's keccak hash).
struct Functor {
  uint32_t value = 0;
  inline bool operator==(const Functor& other) const { return this->value == other.value; }
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
     * Constructor using a bytes::View
     * @param data The bytes::View pointer to convert into a storage key.
     */
    StorageKey(const bytes::View& data) {
      if (data.size() != 52) throw std::invalid_argument("Invalid StorageKey size.");
      std::copy(data.begin(), data.end(), this->begin());
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
