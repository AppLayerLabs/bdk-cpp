#ifndef STRINGS_H
#define STRINGS_H

#include <string>

#include "hex.h"
#include "utils.h"

/**
 * Abstraction of a fixed-size string (`FixedStr<10>` would have
 * *exactly* 10 characters, no more, no less).
 * This class is used as a base for both classes inheriting it
 * (e.g. Hash, Signature, etc.) and aliases (e.g. PrivKey, PubKey, etc.).
 */
template <unsigned N> class FixedStr {
  protected:
    std::string data; ///< Internal string data.

  public:
    /// Constructor.
    inline FixedStr() { this->data.resize(N, 0x00); };

    /// Copy constructor.
    inline FixedStr(const std::string_view data) { this->data = data; this->data.resize(N); }

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

    /**
     * Getter for `data`, but returns the data in hex format.
     * @param strict If `true`, returns the value with an appended "0x" prefix.
     */
    inline const Hex hex(bool strict = false) const { return Hex::fromBytes(this->data, strict); }

    /**
     * Getter for `data`, but returns a read-only copy of the data string.
     * @param pos (optional) Index to start getting chars from. Defaults to the start of the string.
     * @param len (optional) Number of chars to get. Defaults to the whole string.
     * @return A string view of the data string.
     */
    inline const std::string_view view(size_t pos = 0, size_t len = N) const {
      return std::string_view(&this->data[pos], len);
    }

    /// Get the data string's size.
    inline size_t size() const { return this->data.size(); }

    /// Get an iterator pointing to the start of the data string.
    inline const std::string::const_iterator cbegin() const { return this->data.cbegin(); }

    /// Get an iterator pointing to the end of the data string.
    inline const std::string::const_iterator cend() const { return this->data.cend(); }

    /// Equality operator. Checks if both internal strings are the same.
    inline bool operator==(const FixedStr& other) const { return (this->data == other.data); }

    /// Inequality operator. Checks if both internal strings are different.
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

    /**
     * Operator for checking the string's "real emptyness" (all zeroes).
     * @return `true` if string is an empty value, `false` otherwise.
     */
    explicit operator bool() const { return std::any_of(data.begin(), data.end(), [](uint8_t _b) { return _b != 0; }); }
};

/// Abstraction of a 32-byte hash. Inherits `FixedStr<32>`.
class Hash : public FixedStr<32> {
  public:
    using FixedStr<32>::FixedStr; // Using parent constructor
    using FixedStr<32>::operator=; // Using parent operator=

    /**
     * Constructor.
     * @param data The unsigned 256-bit number to convert into a hash string.
     */
    Hash(uint256_t data);

    /// Convert the hash string back to an unsigned 256-bit number.
    const uint256_t toUint256() const;

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
    using FixedStr<65>::FixedStr; // Using parent constructor

    /// Get the first half (32 bytes) of the signature.
    uint256_t r() const;

    /// Get the second half (32 bytes) of the signature.
    uint256_t s() const;

    /// Get the recovery ID (1 byte) of the signature.
    uint8_t v() const;
};

/// Abstraction for a single 20-byte address (e.g. "1234567890abcdef..."). Inherits `FixedStr<20>`.
class Address : public FixedStr<20> {
  public:
    // Using all parent operators.
    using FixedStr<20>::operator==;
    using FixedStr<20>::operator!=;
    using FixedStr<20>::operator<;
    using FixedStr<20>::operator<=;
    using FixedStr<20>::operator>;
    using FixedStr<20>::operator>=;
    using FixedStr<20>::operator=;

    /// Empty constructor for a zero address ("0x0000000000000000000000000000000000000000").
    inline Address() { this->data.resize(20, 0x00); };

    /**
     * Copy constructor.
     * @param add The address itself.
     * @param inBytes If `true`, treats the input as a raw bytes string.
     */
    Address(const std::string_view add, bool inBytes);

    /**
     * Move constructor.
     * @param add The address itself.
     * @param inBytes If `true`, treats the input as a raw bytes string.
     */
    Address(std::string&& add, bool inBytes);

    /// Copy constructor.
    inline Address(const Address& other) { this->data = other.data; }

    /// Move constructor.
    inline Address(Address&& other) : FixedStr<20>(std::move(other.data)) {};

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
      this->data = other.data;
      return *this;
    }

    /// Move assignment operator.
    inline Address& operator=(Address&& other) {
      this->data = std::move(other.data);
      return *this;
    }
};

#endif  // STRINGS_H
