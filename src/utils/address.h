#ifndef BDK_UTILS_ADDRESS_H
#define BDK_UTILS_ADDRESS_H

#include "bytes/range.h"
#include "bytesinterface.h"
#include "evmc/evmc.hpp"
#include "view.h"
#include "zpp_bits.h"

constexpr size_t ADDRESS_SIZE = 20;

/// Abstraction for a single 20-byte address (e.g. "1234567890abcdef...")
class Address : public BytesInterface<Address, ADDRESS_SIZE> {
public:

  /**
   * Constructs an address with all bits clear.
   */
  constexpr Address() : data_() {}

  /**
   * Constructs an address from a given input.
   * The input should construct a BytesInterface.
   * 
   * Implicit construction is enabled when input
   * meet the Initializer requirements.
   */
  template<typename I>
  explicit (not bytes::Initializer<I>)
  constexpr Address(I&& input) : BytesInterface(std::forward<I>(input)) {}

  /**
   * Constructs an address from a initializer list
   */
  constexpr Address(std::initializer_list<Byte> initList) {
    if (initList.size() != ADDRESS_SIZE) {
      throw std::invalid_argument("20 bytes are required to initialize an address object");
    }

    std::ranges::copy(initList, data_.begin());
  }

  /**
   * Returns the hexadecimal checksum of the given address representation.
   * Per [EIP-55](https://eips.ethereum.org/EIPS/eip-55).
   * @param address the address representation
   * @return the checksum hexadecimal
   */
  static Hex checksum(View<Address> address);

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

  /**
   * Returns the beginning iterator of the address.
   * @return the beginning iterator of the address.
   */
  constexpr auto begin() { return data_.begin(); }

  /**
   * Returns the beginning constant iterator of the address.
   * @return the beginning constant iterator of the address.
   */
  constexpr auto begin() const { return data_.begin(); }

private:
  friend zpp::bits::access;
  using serialize = zpp::bits::members<1>;

  std::array<Byte, ADDRESS_SIZE> data_;
};

/**
 * Returns the beginning constant iterator of the bytes range.
 * @param addr the target bytes range
 * @return the beginning constant iterator of the data
 */
constexpr const Byte* begin(const evmc_address& addr) { return addr.bytes; }

/**
 * Returns the beginning iterator of the bytes range.
 * @param addr the target bytes range
 * @return the beginning iterator of the data
 */
constexpr Byte* begin(evmc_address& addr) { return addr.bytes; }

/**
 * Returns the sentinel constant iterator of the bytes range.
 * @param addr the target bytes range
 * @return the sentinel constant iterator of the data
 */
constexpr const Byte* end(const evmc_address& addr) { return addr.bytes + ADDRESS_SIZE; }

/**
 * Returns the sentinel iterator of the bytes range.
 * @param addr the target bytes range
 * @return the sentinel iterator of the data
 */
constexpr Byte* end(evmc_address& addr) { return addr.bytes + ADDRESS_SIZE; }

/**
 * Returns the beginning constant iterator of the bytes range.
 * @param addr the target bytes range
 * @return the beginning constant iterator of the data
 */
constexpr const Byte* begin(const evmc::address& addr) { return addr.bytes; }

/**
 * Returns the beginning iterator of the bytes range.
 * @param addr the target bytes range
 * @return the beginning iterator of the data
 */
constexpr Byte* begin(evmc::address& addr) { return addr.bytes; }

/**
 * Returns the sentinel constant iterator of the bytes range.
 * @param addr the target bytes range
 * @return the sentinel constant iterator of the data
 */
constexpr const Byte* end(const evmc::address& addr) { return addr.bytes + ADDRESS_SIZE; }

/**
 * Returns the sentinel iterator of the bytes range.
 * @param addr the target bytes range
 * @return the sentinel iterator of the data
 */
constexpr Byte* end(evmc::address& addr) { return addr.bytes + ADDRESS_SIZE; }

/**
 * View of a address representation type.
 */
template<>
class View<Address> : public BytesInterface<View<Address>, ADDRESS_SIZE> {
public:

  /**
   * Constructs a address view from the given bytes ranges.
   * 
   * @param range the contiguous and sized range of bytes to be viewed
   * @throw invalid argument exception case the range size is incompatible with the view size.
   */
  template<bytes::DataRange R>
  explicit constexpr View(R&& range) : data_(range) {
    if (const size_t size = std::ranges::size(range); size != ADDRESS_SIZE) {
      throw std::invalid_argument("address view requires exactly 20 bytes, but " + std::to_string(size) + " were given");
    }
  }

  /**
   * Implicitly contructs an address view from a address object
   * 
   * @param address the address object
  */
  constexpr View(const Address& address) : data_(address) {}

  /**
   * Implictly constructs an address view from the evmc address type.
   * 
   * @param address te address object
   */
  constexpr View(const evmc_address& address) : data_(address) {}

  /**
   * Implictly constructs an address view from the evmc address type.
   * 
   * @param address te address object
   */
  constexpr View(const evmc::address& address) : data_(address) {}

  /**
   * Returns the beginning constant iterator of the range
   * @return the beginning constant iterator of the range
   */
  constexpr auto begin() const { return data_.begin(); }

private:
  std::span<const Byte, ADDRESS_SIZE> data_;
};

#endif // BDK_UTILS_ADDRESS_H
