#ifndef BDK_UTILS_HASH_H
#define BDK_UTILS_HASH_H

#include "bytes/range.h"
#include "bytesinterface.h"
#include "evmc/evmc.hpp"
#include "view.h"
#include "zpp_bits.h"

inline constexpr size_t HASH_SIZE = 32;

class Hash : public BytesInterface<Hash, HASH_SIZE> {
public:

  constexpr Hash() : data_() {}

  template<typename I>
  requires std::constructible_from<BytesInterface<Hash, HASH_SIZE>, I&&>
  explicit (not bytes::Initializer<I>)
  constexpr Hash(I&& input) : BytesInterface(std::forward<I>(input)) {}

  explicit Hash(const uint256_t& value);

  explicit operator uint256_t() const;

  constexpr auto begin() { return data_.begin(); }

  constexpr auto begin() const { return data_.begin(); }

private:
  friend zpp::bits::access;
  using serialize = zpp::bits::members<1>;

  std::array<Byte, HASH_SIZE> data_;
};

template<typename T>
struct is_hash_representation : std::is_same<T, Hash> {};

template<>
struct is_hash_representation<evmc_bytes32> : std::true_type {};

template<>
struct is_hash_representation<evmc::bytes32> : std::true_type {};

template<typename T>
inline constexpr bool is_hash_representation_v = is_hash_representation<T>::value;

template<>
class View<Hash> : public BytesInterface<View<Hash>, HASH_SIZE> {
public:

  template<typename R>
  explicit(not is_hash_representation_v<std::remove_cvref_t<R>>)
  constexpr View(R&& input) : data_(std::forward<R>(input)) {}

  constexpr auto begin() const { return data_.begin(); }

  explicit operator uint256_t() const;

private:
  std::span<const Byte, HASH_SIZE> data_;
};

/**
 * This allows the standard algorithms to treat evmc_bytes32 as a range of contiguous bytes.
 * @param hash the constant address representation
 * @return the beginning constant iterator of the address
 */
constexpr const Byte* begin(const evmc_bytes32& hash) { return hash.bytes; }

/**
 * This allows the standard algorithms to treat evmc_bytes32 as a range of contiguous bytes.
 * @param hash the address representation
 * @return the beginning iterator of the address
 */
constexpr Byte* begin(evmc_bytes32& hash) { return hash.bytes; }


/**
 * This allows the standard algorithms to treat evmc_bytes32 as a range of contiguous bytes.
 * @param hash the address representation
 * @return the sentinel constant iterator of the address
 */
constexpr const Byte* end(const evmc_bytes32& hash) { return hash.bytes + HASH_SIZE; }

/**
 * This allows the standard algorithms to treat evmc_bytes32 as a range of contiguous bytes.
 * @param hash the address representation
 * @return the sentinel iterator of the address
 */
constexpr Byte* end(evmc_bytes32& hash) { return hash.bytes + HASH_SIZE; }

/**
 * This allows the standard algorithms to treat evmc::bytes32 as a range of contiguous bytes.
 * @param hash the constant address representation
 * @return the beginning constant iterator of the address
 */
constexpr const Byte* begin(const evmc::bytes32& hash) { return hash.bytes; }

constexpr Byte* begin(evmc::bytes32& hash) { return hash.bytes; }

constexpr const Byte* end(const evmc::bytes32& hash) { return hash.bytes + HASH_SIZE; }

constexpr Byte* end(evmc::bytes32& hash) { return hash.bytes + HASH_SIZE; }

#endif // BDK_UTILS_HASH_H
