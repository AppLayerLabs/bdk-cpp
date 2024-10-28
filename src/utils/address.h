#ifndef BDK_UTILS_ADDRESS_H
#define BDK_UTILS_ADDRESS_H

#include "bytes/range.h"
#include "bytesinterface.h"
#include "evmc/evmc.hpp"
#include "view.h"
#include "zpp_bits.h"

inline constexpr size_t ADDRESS_SIZE = 20;

/// Abstraction for a single 20-byte address (e.g. "1234567890abcdef...")
class Address : public BytesInterface<Address, ADDRESS_SIZE> {
public:

  /**
   * Constructs address with zeroes
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
   * Copy constructor.
   * @param add The address itself.
   * @param inBytes If `true`, treats the input as a raw bytes string.
   * @throw DynamicException if address has wrong size or is invalid.
   */
  Address(const std::string_view add, bool inBytes);

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
 * Base class template for identifying address representation types.
 * Other address types must specialize.
 */
template<typename T>
struct IsAddressType : std::is_same<T, Address> {};

/**
 * Specialization for evmc::address
 */
template<>
struct IsAddressType<evmc::address> : std::true_type {};

/**
 * Specialization for evmc_address
 */
template<>
struct IsAddressType<evmc_address> : std::true_type {};

/**
 * View of a address representation type.
 */
template<>
class View<Address> : public BytesInterface<View<Address>, ADDRESS_SIZE> {
public:

  /**
   * Constructs a address view from the given input.
   * Implicit construction is allowed only for address representation types.
   * e.g. Address, evmc_address, and evmc::address
   */
  template<typename R>
  explicit(not IsAddressType<std::remove_cvref_t<R>>::value)
  constexpr View(R&& address) : data_(std::forward<R>(address)) {}

  /**
   * Returns the beginning constant iterator of the range
   * @return the beginning constant iterator of the range
   */
  constexpr auto begin() const { return data_.begin(); }

private:
  std::span<const Byte, ADDRESS_SIZE> data_;
};

/**
 * This allows the standard algorithms to treat evmc_address as a range of contiguous bytes.
 * @param addr the constant address representation
 * @return the beginning constant iterator of the address
 */
constexpr const Byte* begin(const evmc_address& addr) { return addr.bytes; }

/**
 * This allows the standard algorithms to treat evmc_address as a range of contiguous bytes.
 * @param addr the address representation
 * @return the beginning iterator of the address
 */
constexpr Byte* begin(evmc_address& addr) { return addr.bytes; }


/**
 * This allows the standard algorithms to treat evmc_address as a range of contiguous bytes.
 * @param addr the address representation
 * @return the sentinel constant iterator of the address
 */
constexpr const Byte* end(const evmc_address& addr) { return addr.bytes + ADDRESS_SIZE; }

/**
 * This allows the standard algorithms to treat evmc_address as a range of contiguous bytes.
 * @param addr the address representation
 * @return the sentinel iterator of the address
 */
constexpr Byte* end(evmc_address& addr) { return addr.bytes + ADDRESS_SIZE; }

/**
 * This allows the standard algorithms to treat evmc::address as a range of contiguous bytes.
 * @param addr the constant address representation
 * @return the beginning constant iterator of the address
 */
constexpr const Byte* begin(const evmc::address& addr) { return addr.bytes; }

constexpr Byte* begin(evmc::address& addr) { return addr.bytes; }

constexpr const Byte* end(const evmc::address& addr) { return addr.bytes + ADDRESS_SIZE; }

constexpr Byte* end(evmc::address& addr) { return addr.bytes + ADDRESS_SIZE; }

#endif // BDK_UTILS_ADDRESS_H
