#ifndef BDK_UTILS_HASH_H
#define BDK_UTILS_HASH_H

#include "bytes/range.h"
#include "bytesinterface.h"
#include "evmc/evmc.hpp"
#include "view.h"
#include "zpp_bits.h"

constexpr size_t HASH_SIZE = 32;

/// Abstraction of a 32-byte hash.
class Hash : public BytesInterface<Hash, HASH_SIZE> {
public:

  /**
   * Initializes the hash object with all bits clear.
   */
  constexpr Hash() : data_() {}

  /**
   * Constructs a hash from the given argument. The argument
   * must be able to initialize a BytesInterface. Refer to BytesInterface
   * for checking constructor overloads.
   * 
   * @param input the argument capable of constructing a BytesInterface
   */
  template<typename I>
  requires std::constructible_from<BytesInterface<Hash, HASH_SIZE>, I&&>
  explicit (not bytes::Initializer<I>)
  constexpr Hash(I&& input) : BytesInterface(std::forward<I>(input)) {}

  /**
   * Constructs a hash using the bits of the unsigned 256 bits integer.
   * @param value the 256 bits unsigned integer value
   */
  explicit Hash(const uint256_t& value);

  /**
   * Converts the 256 bits of the hash into a unsigned integer representation.
   * @return the uint256_t representation
   */
  explicit operator uint256_t() const;

  /**
   * @return the beginning iterator of the hash bytes
   */
  constexpr auto begin() { return data_.begin(); }

  /**
   * @return the beginning constant iterator of the hash bytes
   */
  constexpr auto begin() const { return data_.begin(); }

private:
  friend zpp::bits::access;
  using serialize = zpp::bits::members<1>;

  std::array<Byte, HASH_SIZE> data_;
};

/**
 * Returns the beginning constant iterator of the bytes range.
 * @param bytes32 the target bytes range
 * @return the beginning constant iterator of the data
 */
constexpr const Byte* begin(const evmc_bytes32& bytes32) { return bytes32.bytes; }

/**
 * Returns the beginning iterator of the bytes range.
 * @param bytes32 the target bytes range
 * @return the beginning iterator of the data
 */
constexpr Byte* begin(evmc_bytes32& bytes32) { return bytes32.bytes; }


/**
 * Returns the sentinel constant iterator of the bytes range.
 * @param bytes32 the target bytes range
 * @return the sentinel constant iterator of the data
 */
constexpr const Byte* end(const evmc_bytes32& bytes32) { return bytes32.bytes + HASH_SIZE; }

/**
 * Returns the sentinel iterator of the bytes range.
 * @param bytes32 the target bytes range
 * @return the sentinel iterator of the data
 */
constexpr Byte* end(evmc_bytes32& bytes32) { return bytes32.bytes + HASH_SIZE; }


/**
 * Returns the beginning constant iterator of the bytes range.
 * @param bytes32 the target bytes range
 * @return the beginning constant iterator of the data
 */
constexpr const Byte* begin(const evmc::bytes32& hash) { return hash.bytes; }

/**
 * Returns the beginning iterator of the bytes range.
 * @param bytes32 the target bytes range
 * @return the beginning iterator of the data
 */
constexpr Byte* begin(evmc::bytes32& bytes32) { return bytes32.bytes; }

/**
 * Returns the sentinel constant iterator of the bytes range.
 * @param bytes32 the target bytes range
 * @return the sentinel constant iterator of the data
 */
constexpr const Byte* end(const evmc::bytes32& bytes32) { return bytes32.bytes + HASH_SIZE; }

/**
 * Returns the sentinel iterator of the bytes range.
 * @param bytes32 the target bytes range
 * @return the sentinel iterator of the data
 */
constexpr Byte* end(evmc::bytes32& bytes32) { return bytes32.bytes + HASH_SIZE; }

/**
 * Views a type as a hash type.
 */
template<>
class View<Hash> : public BytesInterface<View<Hash>, HASH_SIZE> {
public:

  /**
   * Explicitly constructs a hash view from a borrowed and contiguous
   * range of exactly 32 bytes. Pretty much any type that follows
   * these constraints can be seen as a hash view.
   * 
   * @param range the target sized, contiguous, and borrowed bytes range
   * @throw invalid argument exception if the input range has incorrect size
  */
  template<bytes::DataRange R>
  explicit constexpr View(R&& range) : data_(range) {
    if (const size_t size = std::ranges::size(range); size != HASH_SIZE) {
      throw std::invalid_argument("hash view requires exactly 32 bytes, but " + std::to_string(size) + " were given");
    }
  }

  /**
   * Constructs a view from a const reference to a hash.
   * Implicit construction is allowed.
   * 
   * @param hash the target hash
   */
  constexpr View(const Hash& hash) : data_(hash) {}

  /**
   * Constructs a view from a const reference to a evmc bytes32.
   * Implicit construction is allowed.
   * 
   * @param bytes32 the target bytes
   */
  constexpr View(const evmc_bytes32& bytes32) : data_(bytes32) {}

  /**
   * Constructs a view from a const reference to a evmc bytes32.
   * Implicit construction is allowed.
   * 
   * @param bytes32 the target bytes
   */
  constexpr View(const evmc::bytes32& bytes32) : data_(bytes32) {}

  /**
   * Returns the beginning constant iterator of this range
   * 
   * @return the range beginnig constant iterator
   */
  constexpr auto begin() const { return data_.begin(); }

  /**
   * Casts this hash bits to a unsigned 256-bits integer
   * 
   * @return the unsigned 256-bits integer
   */
  explicit operator uint256_t() const;

private:
  std::span<const Byte, HASH_SIZE> data_;
};

#endif // BDK_UTILS_HASH_H
