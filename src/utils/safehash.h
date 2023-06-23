#ifndef HASH_H
#define HASH_H

#include <memory>
#include <boost/asio/ip/address.hpp>
#include <boost/container_hash/hash.hpp>

#include "strings.h"
#include "utils.h"
#include "tx.h"

/**
 * Custom hashing implementation for use in `std::unordered_map`.
 * Based on [this article](https://codeforces.com/blog/entry/62393).
 *
 * The default `std::unordered_map` implementation uses `uint64_t` hashes,
 * which makes collisions possible by having many Accounts and distributing them
 * in a way that they have the same hash across all nodes.
 *
 * This struct is a workaround for that, it's not perfect because it still uses
 * `uint64_t`, but it's better than nothing since nodes keep different hashes.
 */

struct SafeHash {
  using clock = std::chrono::steady_clock;  ///< Typedef for a less verbose clock.

  /**
   * %Hash a given unsigned integer.
   * Based on [Sebastiano Vigna's original implementation](http://xorshift.di.unimi.it/splitmix64.c).
   * @param i The 64-bit unsigned integer to hash.
   * @returns The hashed data, as a 64-bit unsigned integer.
   */
  inline static uint64_t splitmix(uint64_t i) {
    i += 0x9e3779b97f4a7c15;
    i = (i ^ (i >> 30)) * 0xbf58476d1ce4e5b9;
    i = (i ^ (i >> 27)) * 0x94d049bb133111eb;
    return i ^ (i >> 31);
  }

  /**
   * Wrapper for `splitmix()`.
   * @param i A 64-bit unsigned integer.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(uint64_t i) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(i + FIXED_RANDOM);
  }

  /**
   * Wrapper for `splitmix()`.
   * @param str A regular string.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const std::string& str) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(std::hash<std::string>()(str) + FIXED_RANDOM);
  }

  /**
   * Wrapper for `splitmix()`.
   * @param str A regular string view.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const std::string_view& str) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(std::hash<std::string_view>()(str) + FIXED_RANDOM);
  }

  /**
   * Wrapper for 'splitmix()'.
   * @param bytes A std::vector<uint8_t> object.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const Bytes& bytes) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(boost::hash_range(bytes.begin(), bytes.end()) + FIXED_RANDOM);
  }

  /**
   * Wrapper for 'splitmix()'.
   * @param bytesArr A std::array<uint8_t, N> object.
   * @returns The same as `splitmix()`.
   */
  template <unsigned N>
  size_t operator()(const BytesArr<N>& bytesArr) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(boost::hash_range(bytesArr.begin(), bytesArr.end()) + FIXED_RANDOM);
  }

  /**
   * Wrapper for 'splitmix()'.
   * @param bytesArrView A std::span<const Byte, std::dynamic_extent> object.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const BytesArrView& bytesArrView) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(boost::hash_range(bytesArrView.begin(), bytesArrView.end()) + FIXED_RANDOM);
  }

  /**
   * Wrapper for 'splitmix()'.
   * @param bytesArrMutableView A std::span<Byte, std::dynamic_extent> object
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const BytesArrMutableView& bytesArrMutableView) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(boost::hash_range(bytesArrMutableView.begin(), bytesArrMutableView.end()) + FIXED_RANDOM);
  }

  /**
   * Wrapper for 'splitmix()'
   * @param address A Address (FixedBytes<20>) object
   * @returns The same as `splitmix()`
   */
  size_t operator()(const Address& address) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    // Faster hashing for 20 bytes of data.
    uint32_t const* data = reinterpret_cast<uint32_t const*>(address.raw());
    // 160 / 32 = 5
    return splitmix(boost::hash_range(data, data + 5) + FIXED_RANDOM);
  }

  /**
   * Wrapper for 'splitmix()'.
   * @param functor A functor (FixedBytes<4>) object.
   */
  size_t operator()(const Functor& functor) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
   // Faster hashing for 4 bytes of data.
   uint32_t const* data = reinterpret_cast<uint32_t const*>(functor.raw());
   // 32 / 32 = 1
   return splitmix(boost::hash_range(data, data + 1) + FIXED_RANDOM);
  }

  /**
   * Wrapper for `splitmix()`.
   * @param hash A Hash object.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const Hash& hash) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    // Fast compatible object for hashing 32 bytes of data.
    uint64_t const* data = reinterpret_cast<uint64_t const*>(hash.raw());
    return splitmix(boost::hash_range(data, data + 4) + FIXED_RANDOM);
  }
  /**
   * Wrapper for `splitmix()`.
   * @param tx A TxValidator object.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const TxValidator& tx) const {
    return SafeHash()(tx.hash());
  }

  /**
   * Wrapper for `splitmix()`.
   * @param ptr A shared pointer to any given type.
   * @returns The same as `splitmix()`.
   */
  template <typename T> size_t operator()(const std::shared_ptr<T>& ptr) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(std::hash<std::shared_ptr<T>>()(ptr) + FIXED_RANDOM);
  }

  /**
   * Wrapper for `splitmix()`.
   * @param bytes A %FixedBytes of any size.
   * @returns The same as `splitmix()`.
   */
  template <unsigned N> size_t operator()(const FixedBytes<N>& bytes) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(boost::hash_range(bytes.cbegin(), bytes.cend()) + FIXED_RANDOM);
  }

  /**
   * Wrapper for `splitmix()`.
   * @param a std::unordered_map object.
   * @returns The same as `splitmix()`.
   */
   template <typename Key, typename T>
   size_t operator()(const std::unordered_map<Key, T, SafeHash>& a) const {
     static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
     return splitmix(std::hash<std::unordered_map<Key, T, SafeHash>>()(a) + FIXED_RANDOM);
   }
};

/**
 * [Fowler-Noll-Vo](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function)
 * hash struct used within broadcast messages.
 * This skips compiler optimizations and forces the whole string to be hashed,
 * diminishing the chance of collisions (remember we're using 64-bit hashes).
 */
struct FNVHash {
  /**
   * Call operator.
   * @param s The string to hash.
   */
  size_t operator()(BytesArrView s) const {
    size_t result = 2166136261U;
    BytesArrView::iterator end = s.end();
    for (BytesArrView::iterator it = s.begin(); it != end; it++) {
      result = (16777619 * result) ^ (*it);
    }
    return result;
  }
};

#endif  // HASH_H
