#ifndef BDK_UTILS_FIXEDBYTES_H
#define BDK_UTILS_FIXEDBYTES_H

#include "bytesinterface.h"
#include "hex.h"

/**
 * Abstraction of a fixed-size bytes container.
 * `FixedBytes<10>` would have *exactly* 10 bytes, no more, no less.
 * Used as a base for both aliases (e.g. PrivKey, PubKey, etc.) and classes inheriting it (e.g. Hash, Signature, etc.).
 */
template <size_t N>
class FixedBytes : public BytesInterface<FixedBytes<N>, N> {
public:
  constexpr FixedBytes() : data_() {}

  constexpr FixedBytes(std::initializer_list<Byte> initList) {
    if (initList.size() != N)
      throw DynamicException("Given initializer list of size " + std::to_string(initList.size()) + 
        " is not suitable for initializing a FixedBytes<" + std::to_string(N) + ">");

    std::ranges::copy(initList, data_.begin());
  }  

  constexpr FixedBytes(bytes::Initializer auto&& initializer) { initializer.to(data_); }

  explicit constexpr FixedBytes(const bytes::Range auto& input) {
    if (const size_t size = std::ranges::size(input); size != N) {
      throw std::invalid_argument("Given bytes range of size " + std::to_string(size) +
        " is not suitable for initializing a FixedBytes<" + std::to_string(N) + ">");
    }

    std::ranges::copy(input, data_.begin());
  }

  constexpr auto begin() { return data_.begin(); }

  constexpr auto begin() const { return data_.begin(); }

private:
  BytesArr<N> data_;

  friend zpp::bits::access;
  using serialize = zpp::bits::members<1>;
};

#endif // BDK_UTILS_FIXEDBYTES_H
