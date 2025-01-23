#ifndef BDK_UTILS_SIGNATURE_H
#define BDK_UTILS_SIGNATURE_H

#include "bytes/range.h"
#include "bytesinterface.h"
#include "evmc/evmc.hpp"
#include "hash.h"
#include "view.h"

constexpr size_t SIGNATURE_SIZE = 65;

/**
 * Base class for all signature types. Should NOT be used for polymorphism.
 * It's helper class template aimed to define a common interface for all signature types.
 * This class uses CRTP.
 */
template<typename T>
class SignatureInterface : public BytesInterface<T, SIGNATURE_SIZE> {
public:
  using BytesInterface<T, SIGNATURE_SIZE>::BytesInterface;

  /**
   * @return get the first half (32 bytes) of the signature.
   */
  constexpr uint256_t r() const {
    return static_cast<uint256_t>(View<Hash>(self() | std::views::take(HASH_SIZE)));
  }

  /**
   * @return get the second half (32 bytes) of the signature.
   */
  constexpr uint256_t s() const {
    return static_cast<uint256_t>(View<Hash>(
      self() | std::views::drop(HASH_SIZE) | std::views::take(HASH_SIZE)));
  }

  /**
   * @return get the recovery ID (1 byte) of the signature.
   */
  constexpr uint8_t v() const {
    return *std::ranges::rbegin(self());
  }

private:
  constexpr const T& self() const { return static_cast<const T&>(*this); }
};

/// Abstraction of a 65-byte ECDSA signature.
class Signature : public SignatureInterface<Signature> {
public:
  /**
   * Constructs a signature object with all bits clear
   */
  constexpr Signature() : data_() {}

    /**
   * Constructs a hash from the given argument. The argument
   * must be able to initialize a BytesInterface. Refer to BytesInterface
   * for checking constructor overloads.
   * 
   * @param input the argument capable of constructing a BytesInterface
   */
  template<typename I>
  explicit (not bytes::Initializer<I>)
  constexpr Signature(I&& input) : SignatureInterface(std::forward<I>(input)) {}

  /**
   * @return the beginning iterator of the bytes range
   */
  constexpr auto begin() { return data_.begin(); }

  /**
   * @return the beginning constant iterator of the bytes range
   */
  constexpr auto begin() const { return data_.begin(); }

private:
  std::array<Byte, SIGNATURE_SIZE> data_;
};

template<>
class View<Signature> : public SignatureInterface<View<Signature>> {
public:
  /**
   * Constructs a signature view from the given bytes range of 65 bytes.
   * 
   * @param range a contiguous and sized byte range of exactly 65 bytes
   * @throw invalid argument exception if the input range does not have the correct size
   */
  template<bytes::DataRange R>
  explicit constexpr View(R&& range) : data_(range) {
    if (const size_t size = std::ranges::size(range); size != HASH_SIZE) {
      throw std::invalid_argument("signature view requires exactly 65 bytes, but " + std::to_string(size) + " were given");
    }
  }

  /**
   * Constructs a signature view from a signature object.
   * 
   * @param the signature object
   */
  constexpr View(const Signature& signature) : data_(signature) {}

  /**
   * @return the beginning constant iterator of the signature bytes range
   */
  constexpr auto begin() const { return data_.begin(); }

private:
  std::span<const Byte, SIGNATURE_SIZE> data_;
};

#endif // BDK_UTILS_SIGNATURE_H
