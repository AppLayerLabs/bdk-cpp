#ifndef BYTES_INITIALIZER_H
#define BYTES_INITIALIZER_H

//#include <functional> // TODO: probably not used

#include "view.h" // range.h -> ranges -> concepts

namespace bytes {

/**
 * The concept of a initializer. It should be able to
 * initialize any sized span of bytes
*/
template<typename T>
concept Initializer = requires (const T& a) {
  a.to(std::declval<Span>());
};

/**
 * An Initializer with a specific size target. It's only able to
 * initialize 
*/
template<typename T>
concept SizedInitializer = Initializer<T> && requires (const T& a) {
  a.to(std::declval<Byte*>());
  { a.size() } -> std::convertible_to<std::size_t>;
};


template<std::invocable<Span> F>
class BasicInitializer {
public:
  constexpr explicit BasicInitializer(F func)
    : func_(std::move(func)) {}

  constexpr void to(Span span) const { std::invoke(func_, span); }

private:
  F func_;
};


template<std::invocable<Byte*> F>
class BasicSizedInitializer {
public:
  constexpr BasicSizedInitializer(F func, std::size_t size)
   : func_(std::move(func)), size_(size) {}

  constexpr size_t size() const { return size_; }

  constexpr void to(Byte *dest) const { std::invoke(func_, dest); }

  constexpr void to(Span dest) const {
    if (dest.size() != size()) [[unlikely]] {
      throw std::invalid_argument(std::string("span size (") + std::to_string(dest.size()) + 
        ") incompatible with initializer size (" + std::to_string(size()) + ")");
    }

    to(dest.data());
  }

private:
  F func_;
  std::size_t size_;
};

template<typename T>
constexpr Initializer auto makeInitializer(T func) {
  return BasicInitializer<T>(std::move(func));
}

template<typename T>
constexpr SizedInitializer auto makeInitializer(std::size_t size, T func) {
  return BasicSizedInitializer<T>(std::move(func), size);
}

} // namespace bytes

#endif // BYTES_INITIALIZER_H
