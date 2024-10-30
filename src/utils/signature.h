#ifndef BDK_UTILS_SIGNATURE_H
#define BDK_UTILS_SIGNATURE_H

#include "bytes/range.h"
#include "bytesinterface.h"
#include "evmc/evmc.hpp"
#include "hash.h"
#include "view.h"

inline constexpr size_t SIGNATURE_SIZE = 65;

template<typename T>
class BaseSignature : public BytesInterface<T, SIGNATURE_SIZE> {
public:
  using BytesInterface<T, SIGNATURE_SIZE>::BytesInterface;

  uint256_t r() const {
    return static_cast<uint256_t>(View<Hash>(self() | std::views::take(HASH_SIZE)));
  }

  uint256_t s() const {
    return static_cast<uint256_t>(View<Hash>(
      self() | std::views::drop(HASH_SIZE) | std::views::take(HASH_SIZE)));
  }

  uint8_t v() const {
    return *std::ranges::rbegin(self());
  }

private:
  constexpr const T& self() const { return static_cast<const T&>(*this); }
};

class Signature : public BaseSignature<Signature> {
public:
  constexpr Signature() : data_() {}

  template<typename I>
  explicit (not bytes::Initializer<I>)
  constexpr Signature(I&& input) : BaseSignature(std::forward<I>(input)) {}

  constexpr auto begin() { return data_.begin(); }

  constexpr auto begin() const { return data_.begin(); }

private:
  std::array<Byte, SIGNATURE_SIZE> data_;
};

template<>
class View<Signature> : public BaseSignature<View<Signature>> {
public:
  template<typename R>
  explicit(not std::same_as<std::remove_cvref_t<R>, Signature>)
  constexpr View(R&& input) : data_(std::forward<R>(input)) {}

  constexpr auto begin() const { return data_.begin(); }

private:
  std::span<const Byte, SIGNATURE_SIZE> data_;
};

#endif // BDK_UTILS_SIGNATURE_H
