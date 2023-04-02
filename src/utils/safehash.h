#ifndef HASH_H
#define HASH_H

#include <memory>
#include <boost/asio/ip/address.hpp>

#include "strings.h"
#include "utils.h"
#include "tx.h"

/**
 * Custom hashing implementation for use in `std::unordered_map`.
 * Based on [this article](https://codeforces.com/blog/entry/62393).
 * The default `std::unordered_map` implementation uses `uint64_t` hashes,
 * which makes collisions possible by having many Accounts and distributing them
 * in a way that they have the same hash across all nodes.
 * This struct is a workaround for that, it's not perfect because it still uses
 * `uint64_t`, but it's better than nothing since nodes keeps different hashes.
 */
struct SafeHash {
  using clock = std::chrono::steady_clock;

  /**
   * Hash a given unsigned integer.
   * Based on [Sebastiano Vigna's original implementation](http://xorshift.di.unimi.it/splitmix64.c).
   * @param i A 64-bit unsigned integer.
   * @returns The hashed data as a 64-bit unsigned integer.
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
   * @param add An Address object.
   * @returns The same as `splitmix()`.
   */
  size_t operator()(const Address& add) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(std::hash<std::string>()(add.get()) + FIXED_RANDOM);
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
   * Wrapper for `splitmix()`.
   * @param tx a TxValidator object
   * @returns The same as `splitmix()`.
   */

  size_t operator()(const TxValidator& tx) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(std::hash<std::string>()(tx.hash().get()) + FIXED_RANDOM);
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
   * @param str A %FixedStr of any size.
   * @returns The same as `splitmix()`.
   */
  template <unsigned N> size_t operator()(const FixedStr<N>& str) const {
    static const uint64_t FIXED_RANDOM = clock::now().time_since_epoch().count();
    return splitmix(std::hash<std::string>()(str.get()) + FIXED_RANDOM);
  }
};

/// Hash to be used within broadcast messages.
/// This skips compilator optimizations and forces the whole string to be hashed.
/// Diminishing the chance of collisions (remember, we're using 64-bit hashes).
struct FNVHash
{
  size_t operator()(std::string_view s) const
  {
    size_t result = 2166136261U;
    std::string_view::const_iterator end = s.end();
    for (std::string_view::const_iterator iter = s.begin(); iter != end; ++iter ) {
      result = (16777619 * result) ^ static_cast<unsigned char>(*iter);
    }
    return result;
  }
};

#endif  // HASH_H
