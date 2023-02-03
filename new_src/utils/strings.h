#ifndef STRINGS_H
#define STRINGS_H

#include <string>

#include "hex.h"
#include "utils.h"

/**
 * Abstraction of a fixed-size string.
 * e.g. "FixedStr<10>" would have *exactly* 10 characters, no more, no less.
 * This class is used as a base for both classes inheriting it
 * (e.g. Hash, Signature, etc.) and aliases (e.g. PrivKey, Pubkey, etc.).
 */
template <unsigned N> class FixedStr {
  protected:
    std::string data; ///< Internal string data.

  public:
    /// Constructor.
    inline FixedStr() { this->data.resize(N, 0x00); };

    /// Copy constructor.
    inline FixedStr(const std::string_view& data) { this->data = data; this->data.resize(N); }

    /// Move constructor.
    inline FixedStr(std::string&& data) { this->data = std::move(data); this->data.resize(N); }

    /// Copy constructor.
    inline FixedStr(const FixedStr& other) { this->data = other.data; }

    /// Move constructor.
    inline FixedStr(FixedStr&& other) noexcept { this->data = std::move(other.data); }

    /// Getter for `data`.
    inline const std::string& get() const { return this->data; }

    /// Getter for `data`, but returns the raw C-style string.
    inline const char* raw() const { return this->data.data(); }

    /// Getter for `data`, but returns the data in hex format.
    inline const Hex hex() const { return Hex::fromBytes(this->data); }

    /**
     * Getter for `data`, but returns a read-only copy of the data string.
     * @param size (optional) Number of chars to get. Defaults to the whole string.
     * @param offset (optional) Index to start getting chars from. Defaults to start of the string.
     * @return A string view of the data string.
     */
    inline const std::string_view view(const size_t& size = N, const size_t& offset = 0) const {
      return std::string_view(&this->data[offset], size);
    }

    /**
     * Check if the data string is empty.
     * @returns `true` if the data string is empty, `false` otherwise.
     */
    inline bool empty() const { return this->data.empty(); }

    /// Get the data string's size.
    inline size_t size() const { return this->data.size(); }

    /// Get an iterator poiinting to the start of the data string.
    inline const std::string::const_iterator cbegin() const { return this->data.cbegin(); }

    /// Get an iterator poiinting to the end of the data string.
    inline const std::string::const_iterator cend() const { return this->data.cend(); }

    /// Equality operator.
    inline bool operator==(const FixedStr& other) const { return (this->data == other.data); }

    /// Inquality operator.
    inline bool operator!=(const FixedStr& other) const { return (this->data != other.data); }

    /// Lesser operator. Does a lexicographical check on both data strings.
    inline bool operator<(const FixedStr& other) const { return (this->data < other.data); }

    /// Greater-or-equal operator. Does a lexicographical check on both data strings.
    inline bool operator>=(const FixedStr& other) const { return (this->data >= other.data); }

    /// Lesser-or-equal operator. Does a lexicographical check on both data strings.
    inline bool operator<=(const FixedStr& other) const { return (this->data <= other.data); }

    /// Greater operator. Does a lexicographical check on both data strings.
    inline bool operator>(const FixedStr& other) const { return (this->data > other.data); }

    /// Copy assignment operator.
    inline FixedStr& operator=(const FixedStr& str) {
      if (&str != this) this->data = str.data; return *this;
    }

    /// Move assignment operator.
    inline FixedStr& operator=(FixedStr&& str) {
      if (&str != this) this->data = std::move(str.data); return *this;
    }

    /// Indexing operator.
    inline const char& operator[](const size_t pos) const { return data[pos]; }
};

/// Abstraction of a 32-byte hash. Inherits `FixedStr<32>`.
class Hash : public FixedStr<32> {
  public:
    using FixedStr<32>::FixedStr; ///< Using parent constructor.
    using FixedStr<32>::operator=; ///< Using parent operator=.

    /**
     * Constructor.
     * @param data The unsigned 256-bit number to convert into a hash string.
     */
    Hash(uint256_t data) : FixedStr<32>(Utils::uint256ToBytes(data)) {};

    /// Convert the hash string back to an unsigned 256-bit number.
    inline const uint256_t toUint256() const { return Utils::bytesToUint256(data); }

    /// Generate a random 32-byte/256-bit hash.
    inline static Hash random() {
      Hash h;
      RAND_bytes((unsigned char*)h.data.data(), 32);
      return h;
    }
};

/// Abstraction of a 65-byte ECDSA signature. Inherits `FixedStr<65>`.
class Signature : public FixedStr<65> {
  public:
    using FixedStr<65>::FixedStr; ///< Using parent constructor.

    /// Get the first half (32 bytes) of the signature.
    inline FixedStr<32> r() const { return FixedStr<32>(this->data.substr(0, 32)); };

    /// Get the second half (32 bytes) of the signature.
    inline FixedStr<32> s() const { return FixedStr<32>(this->data.substr(32, 32)); };

    /// Get the recovery ID (1 byte).
    inline FixedStr<1> v() const { return FixedStr<1>(this->data.substr(64, 1)); };
};

/// Abstraction for a single 20-byte address (e.g. "1234567890abcdef..."). Inherits `FixedStr<20>`.
class Address : public FixedStr<20> {
  public:
    /**
     * Copy constructor.
     * @param add The address itself.
     * @param fromRPC If `true`, considers the address is a string, patches it
     *                and stores it as bytes. If `false`, considers the address
     *                is in raw bytes format.
     */
    inline Address(const std::string& add, bool fromRPC) { this->data = Hex(add, fromRPC).bytes(); }

    /// Overload of copy constructor that accepts a string_view.
    inline Address(const std::string_view& add, bool fromRPC) { this->data = Hex(add, fromRPC).bytes(); }

    /**
     * Move constructor.
     * @param add The address itself.
     * @param fromRPC If `true`, considers the address is a string, patches it
     *                and stores it as bytes. If `false`, considers the address
     *                is in raw bytes format.
     */
    inline Address(std::string&& add, bool fromRPC) { this->data = Hex(std::move(add), fromRPC).bytes(); }

    /// Copy constructor.
    inline Address(const Address& other) { this->data = other.data; }

    /// Move constructor.
    inline Address(Address&& other) : FixedStr<20>(std::move(other.data)) {};
};

#endif  // STRINGS_H
