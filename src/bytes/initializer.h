/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BYTES_INITIALIZER_H
#define BYTES_INITIALIZER_H

#include "view.h" // range.h -> ranges -> concepts

/// Namespace for bytes-related functionalities.
namespace bytes {
  /// The concept of an initializer. It should be able to initialize any sized span of bytes.
  template <typename T> concept Initializer = requires (const T& a) { a.to(std::declval<Span>()); };

  /// The concept of an initializer with a specific size target. It should be able to only initialize a fixed-size span of bytes.
  template <typename T> concept SizedInitializer = Initializer<T> && requires (const T& a) {
    a.to(std::declval<Byte*>());
    { a.size() } -> std::convertible_to<std::size_t>;
  };

  /// The initializer class.
  template<std::invocable<Span> F> class BasicInitializer {
    private:
      F func_;  ///< The internal function to use for initializing.

    public:
      /**
       * Constructor.
       * @param func The internal function to use for initializing.
       */
      constexpr explicit BasicInitializer(F func) : func_(std::move(func)) {}

      /**
       * Invoke the internal function with a given span of bytes.
       * @param span The span of bytes to use for invoking.
       */
      constexpr void to(Span span) const { std::invoke(func_, span); }
  };

  /// The initializer class with a specific size target.
  template<std::invocable<Byte*> F> class BasicSizedInitializer {
    private:
      F func_;  ///< The internal function to use for initializing.
      std::size_t size_;  ///< The fixed size of the span.

    public:
      /**
       * Constructor.
       * @param func The internal function to use for initializing.
       * @param size The fixed size of the span.
       */
      constexpr BasicSizedInitializer(F func, std::size_t size) : func_(std::move(func)), size_(size) {}

      /// Get the size of the span.
      constexpr size_t size() const { return size_; }

      /**
       * Invoke the internal function with a given span of bytes.
       * @param dest A pointer to a given byte that will be used for invoking.
       */
      constexpr void to(Byte* dest) const { std::invoke(func_, dest); }

      /**
       * Overload of to() that uses a byte span instead of a single byte.
       * @param dest The span of bytes to use for invoking.
       * @throw std::invalid_argument if sizes do not match.
       */
      constexpr void to(Span dest) const {
        if (dest.size() != size()) [[unlikely]] {
          throw std::invalid_argument(std::string("span size (") + std::to_string(dest.size()) +
            ") incompatible with initializer size (" + std::to_string(size()) + ")");
        }
        to(dest.data());
      }
    };

  /**
   * Helper templated function to initialize a bytes span.
   * @tparam T The internal type of the span.
   * @param func The initializer function.
   * @return The respective basic initializer object.
   */
  template<typename T> constexpr Initializer auto makeInitializer(T func) {
    return BasicInitializer<T>(std::move(func));
  }

  /**
   * Helper templated function to initialize a fixed-size bytes span.
   * @tparam T The internal type of the span.
   * @param size The fixed size of the bytes span.
   * @param func The initializer function.
   * @return The respective basic initializer object.
   */
  template<typename T> constexpr SizedInitializer auto makeInitializer(std::size_t size, T func) {
    return BasicSizedInitializer<T>(std::move(func), size);
  }
} // namespace bytes

#endif // BYTES_INITIALIZER_H
