#ifndef BDK_UTILS_BYTESINTERFACE_H
#define BDK_UTILS_BYTESINTERFACE_H

#include <cstring>
#include <compare>
#include <ranges>

#include "bytes/initializer.h"
#include "hex.h"

/**
 * CRTP helper class template for defining a range of bytes.
 * 
 * Derived classes of static extent must only implement begin().
 * Derived classes of dynamic extent must also implement end().
 * 
 * This class will generate all common functions of bytes range,
 * such as size(), data(), operator[], and comparison operators.
 * 
 * This class is heavily based on std::ranges::view_interface.
 */
template<typename T, size_t N = std::dynamic_extent>
class BytesInterface {
public:
  constexpr BytesInterface() = default;

  /**
   * Helper constructor from bytes initializer.
   */
  explicit constexpr BytesInterface(bytes::Initializer auto&& initializer) {
    initializer.to(self());
  }

  /**
   * Construction by copying another bytes range.
   */
  explicit constexpr BytesInterface(const bytes::Range auto& data)
  requires (N != std::dynamic_extent) {
    if (const size_t size = std::ranges::size(data); size != N)
        throw std::invalid_argument("Given bytes range of size " + std::to_string(size) + 
          " is not suitable for initializing a FixedBytes<" + std::to_string(N) + ">");

    std::ranges::copy(data, self().begin());
  }

  /**
   * Default equality operator for ranges of bytes.
   */
  friend constexpr bool operator==(const T& lhs, const T& rhs) {
    return std::ranges::equal(lhs, rhs);
  }


  /**
   * Default "space-ship" operator for ranges of bytes. This will generate
   * operator<, operator<=, operator>, and operator>=.
   */
  friend constexpr std::strong_ordering operator<=>(const T& lhs, const T& rhs) {
    assert(std::ranges::size(lhs) == std::ranges::size(rhs));
    
    const int r = std::memcmp(std::ranges::data(lhs), std::ranges::data(rhs), std::ranges::size(lhs));

    if (r < 0) {
      return std::strong_ordering::less;
    } else if (r > 0) {
      return std::strong_ordering::greater;
    } else {
      return std::strong_ordering::equivalent;
    }
  }

  /**
   * @return constant beginning iterator of the range.
   */
  constexpr auto cbegin() const { return self().begin(); }

  /**
   * @return sentinel iterator of the range.
   */
  constexpr auto end() requires (N != std::dynamic_extent) { return self().begin() + N; }

  /**
   * @return sentinel iterator of the range.
   */
  constexpr auto end() const requires (N != std::dynamic_extent) { return self().begin() + N; }

  /**
   * @return sentinel constant iterator of the range.
   */
  constexpr auto cend() const { return end(); }

  /**
   * @return size of the range.
   */
  constexpr size_t size() const {
    if constexpr (N == std::dynamic_extent)
      return std::distance(self().begin(), self().end());
    else
      return N;
  }

  /**
   * @return pointer to the beginning of the range.
   */
  constexpr auto data() { return std::to_address(self().begin()); }

  /**
   * @return pointer to the beginning of the range.
   */
  constexpr auto data() const { return std::to_address(self().begin()); }

  /**
   * @return (usually) a reference to the element at given index.
   */
  constexpr decltype(auto) operator[](size_t i) { return *(self().begin() + i); }

  /**
   * @return (usually) a reference to the element at given index.
   */
  constexpr decltype(auto) operator[](size_t i) const { return *(self().begin() + i); }

  /**
   * @return false if all elements are 0, true otherwise.
   */
  explicit constexpr operator bool() const {
    return std::ranges::any_of(self(), [] (Byte b) { return b != 0; });
  }

  /**
   * @return hexadecimal representation of the range
   */
  Hex hex(bool strict = false, bool upper = false) const { return Hex::fromBytes(self(), strict, upper); }

  /**
   * @param pos the starting position of the view
   * @param len the length of the view
   * @return a view from the given position and with the given length
   * @throw out of range exception if given position or length are invalid
   */
  constexpr View<Bytes> view(size_t pos, size_t len) const {
    const size_t real_len = std::min(len, N - pos);

    if (pos + real_len > size()) {
      throw std::out_of_range("len greater than size");
    }

    return View<Bytes>(self().begin() + pos, self().begin() + pos + real_len);
  }

  constexpr View<Bytes> view(size_t pos) const { return view(pos, size()); }

  constexpr View<Bytes> view() const { return view(0); }

  /**
   * Transforms the range to Bytes
   */
  Bytes asBytes() const {
    Bytes res;
    res.resize(size());
    std::ranges::copy(self(), res.begin());
    return res;
  }

private:
  constexpr T& self() { return static_cast<T&>(*this); }
  constexpr const T& self() const { return static_cast<const T&>(*this); }
};

#endif // BDK_UTILS_BYTESINTERFACE_H
