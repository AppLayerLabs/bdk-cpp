#ifndef BYTES_JOIN_H
#define BYTES_JOIN_H

#include "view.h"
#include "range.h"

#include "utils/dynamicexception.h"

namespace bytes {

/**
 * The initializer for joined bytes ranges
*/
template<typename... Ts>
class JoinInitializer {
private:
  std::tuple<Ts...> buffers_;

  /// the number of ranges of this initializer
  inline static constexpr size_t N = std::tuple_size_v<std::tuple<Ts...>>;

  /**
   * Copies sequentially all the bytes from the bytes ranges of this.
   * 
   * @param st the starting iterator that should receive the data
   * @param en the ending iterator for preveting boundaries overflows
   * @throws an exception if the tuple of buffers have more or less bytes than required
  */
  template<size_t I, typename It>
  constexpr void copyAllTo(It st, It en) {
    if constexpr (I < N) {
      const auto& source = std::get<I>(buffers_);

      if (std::ranges::size(source) > std::distance(st, en))
        throw DynamicException("The given buffer does not have enough capacity");

      st = std::copy(source.begin(), source.end(), st);
      copyAllTo<I + 1>(st, en);
    } else {
      if (st != en)
        throw DynamicException("The given buffer has more capacity than necessary");
    }
  }

  /// Inner recursive template function for calculating the size sum of all data ranges
  template<size_t I>
  constexpr size_t totalSize() const {
    if constexpr (I < N) {
      return std::ranges::size(std::get<I>(buffers_)) + totalSize<I + 1>();
    } else {
      return 0;
    }
  }

public:

  /**
   * Constructs a JoinInitializer with the given buffers. rvalue references will be owned by
   * the JoinInitializer to avoid dangling data. This also means thay copying the initializer
   * may also copy its owned buffers, which could be expensive.
  */
  template<DataRange... Us>
  constexpr explicit JoinInitializer(Us&&... buffers) : buffers_(std::forward<Us>(buffers)...) {}

  /**
   * Initializes the target Span with the sequence of buffers of the initializer. The size of the
   * target Span must be exactly the same as sum of the JoinInitializer buffers. An exception will
   * be throw otherwise.
   * 
   * @param target a span that will have the buffers data copied to
   * @throws an exception case the target size does not match the buffers sizes
  */
  constexpr void operator()(Span target) {
    copyAllTo<0>(target.begin(), target.end());
  }

  /**
   * Calculates the sum of the buffers sizes
   * @return the sum of the buffers sizes
  */
  constexpr size_t size() const {
    return totalSize<0>();
  }
};

/**
 * Creates an initializer that lazily joins a series of contiguous ranges of bytes
 * to a single bytes container. Note that the ranges are taken as forwarding references,
 * thus the initializer may own the received range until it is used to initialize
 * the target bytes container.
 * 
 * @param args variadic data ranges to be joined
 * @return a lazy initializer for the joined data
*/
template<DataRange... Ts>
constexpr auto join(Ts&&... args) {
  return JoinInitializer<Ts...>(std::forward<Ts>(args)...);
}

} // namespace bytes

#endif // BYTES_JOIN_H
