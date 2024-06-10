/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef HASH_H
#define HASH_H

#include <memory>
#include <boost/asio/ip/address.hpp>

#include "../libs/wyhash.h"
#include "strings.h"
#include "utils.h"
#include "tx.h"

/**
 * Custom hashing implementation for use in `boost::unordered_flat_map`
 * We use the highest and fastest quality hash function available for size_t (64-bit) hashes (Wyhash)
 */
struct SafeHash {
  ///@{
  /** Wrapper for `splitmix()`. */
  size_t operator()(const uint64_t& i) const {
    return wyhash(std::bit_cast<const void*>(&i), sizeof(i), 0, _wyp);
  }

  size_t operator()(const std::string& str) const {
    return wyhash(str.c_str(), str.size(), 0, _wyp);
  }

  size_t operator()(const std::string_view& str) const {
    return wyhash(str.data(), str.size(), 0, _wyp);
  }

  size_t operator()(const Bytes& bytes) const {
    return wyhash(std::bit_cast<const void*>(bytes.data()), bytes.size(), 0, _wyp);
  }

  template <unsigned N> size_t operator()(const BytesArr<N>& bytesArr) const {
    return wyhash(std::bit_cast<const void*>(bytesArr.data()), bytesArr.size(), 0, _wyp);
  }

  size_t operator()(const bytes::View& bytesArrView) const {
    return wyhash(std::bit_cast<const void*>(bytesArrView.data()), bytesArrView.size(), 0, _wyp);
  }

  size_t operator()(const Address& address) const {
    return wyhash(std::bit_cast<const void*>(address.data()), address.size(), 0, _wyp);
  }

  size_t operator()(const Functor& functor) const {
    return functor.value; // Functor is already a hash. Just return it.
  }

  size_t operator()(const Hash& hash) const {
    return wyhash(std::bit_cast<const void*>(hash.data()), hash.size(), 0, _wyp);
  }

  size_t operator()(const TxValidator& tx) const { return SafeHash()(tx.hash()); }

  template <typename T> size_t operator()(const std::shared_ptr<T>& ptr) const {
    return SafeHash()(*ptr->get());
  }

  template <unsigned N> size_t operator()(const FixedBytes<N>& bytes) const {
    return wyhash(std::bit_cast<const void*>(bytes.data()), bytes.size(), 0, _wyp);
  }

  template <typename Key, typename T> size_t operator()(const boost::unordered_flat_map<Key, T, SafeHash>& a) const {
    // TODO: replace this with wyhash somehow.
    return splitmix(std::hash<boost::unordered_flat_map<Key, T, SafeHash>>()(a));
  }

  size_t operator()(const std::pair<boost::asio::ip::address, uint16_t>& nodeId) const {
    // Make it compatible with SafeHash<Bytes>.
    Bytes bytes;
    if (nodeId.first.is_v4()) {
      Utils::appendBytes(bytes, nodeId.first.to_v4().to_bytes());
    } else {
      Utils::appendBytes(bytes, nodeId.first.to_v6().to_bytes());
    }
    Utils::appendBytes(bytes, Utils::uint16ToBytes(nodeId.second));
    return SafeHash()(bytes);
  }
  ///@}
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
  size_t operator()(bytes::View s) const {
    size_t result = 2166136261U;
    for (auto it = s.begin(); it != s.end(); it++) result = (16777619 * result) ^ (*it);
    return result;
  }
};

#endif  // HASH_H
