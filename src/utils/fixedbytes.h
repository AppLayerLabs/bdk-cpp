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

  /**
   * Constructs a fixed bytes container with all bits clear
   */
  constexpr FixedBytes() : data_() {}

  /**
   * Constructs a fixed bytes container from the given initializer list
   * 
   * @param initList the initalizer list
   * @throw invalid argument exception if the initializer list size does not match the container size
   */
  constexpr FixedBytes(std::initializer_list<Byte> initList) {
    if (initList.size() != N)
      throw std::invalid_argument("Given initializer list of size " + std::to_string(initList.size()) + 
        " is not suitable for initializing a FixedBytes<" + std::to_string(N) + ">");

    std::ranges::copy(initList, data_.begin());
  }  

  /**
   * Constructs a fixed bytes container from the given bytes initializer
   * 
   * @param initializer the bytes initializer
   */
  constexpr FixedBytes(bytes::Initializer auto&& initializer) { initializer.to(data_); }

  /**
   * Constructs a fixed bytes container by copying the bytes from the given range.
   * 
   * @param input the input bytes
   * @throw invalid argument exception if the input size does not match the container size
   */
  explicit constexpr FixedBytes(const bytes::Range auto& input) {
    if (const size_t size = std::ranges::size(input); size != N) {
      throw std::invalid_argument("Given bytes range of size " + std::to_string(size) +
        " is not suitable for initializing a FixedBytes<" + std::to_string(N) + ">");
    }

    std::ranges::copy(input, data_.begin());
  }

  /**
   * @return the beginning iterator of this container bytes
   */
  constexpr auto begin() { return data_.begin(); }

  /**
   * @return the beginning constant iterator of this container bytes
   */
  constexpr auto begin() const { return data_.begin(); }

private:
  BytesArr<N> data_;

  friend zpp::bits::access;
  using serialize = zpp::bits::members<1>;
};

#endif // BDK_UTILS_FIXEDBYTES_H
