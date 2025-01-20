#ifndef _BUCKETCACHE_H_
#define _BUCKETCACHE_H_

#include <unordered_map>
#include <mutex>
#include <array>
#include <optional>

/**
 * A cache backed by two rotating unordered maps of K to V.
 */
template <
  typename K,
  typename V,
  typename Hasher = std::hash<K>,
  typename KeyEqual = std::equal_to<K>
>
class BucketCache {
public:

  /**
   * Construct a BucketCache with two rotating buckets.
   * @param cacheSize Cache size (maximum size of one bucket).
   */
  explicit BucketCache(uint64_t cacheSize)
    : cacheSize_(cacheSize),
      activeBucket_(0)
  {
  }

  /**
   * Save an entry in the cache, possibly causing a bucket clear and flip.
   * @param key The key to store.
   * @param value The value to store.
   */
  void put(const K& key, const V& value) {
    std::scoped_lock lock(cacheMutex_);
    auto& bucket = cache_[activeBucket_];
    bucket[key] = value;
    if (cacheSize_ > 0 && bucket.size() >= cacheSize_) {
      activeBucket_ = 1 - activeBucket_;
      cache_[activeBucket_].clear();
    }
  }

  /**
   * Get an entry from the cache.
   * @param key The key to look up.
   * @return The value associated with the key, or an empty std::optional if not found.
   */
  std::optional<V> get(const K& key) const {
    std::unique_lock lock(cacheMutex_);
    for (int i = 0; i < 2; ++i) {
      const auto& bucket = cache_[(activeBucket_ + i) % 2];
      auto it = bucket.find(key);
      if (it != bucket.end()) {
        return it->second;
      }
    }
    return std::nullopt;
  }

  /**
   * Set a new maximum size for a cache bucket.
   * If `size` is 0, both buckets are immediately cleared.
   * @param size The new size.
   */
  void resize(uint64_t size) {
    std::scoped_lock lock(cacheMutex_);
    cacheSize_ = size;
    if (cacheSize_ == 0) {
      cache_[0].clear();
      cache_[1].clear();
      activeBucket_ = 0;
    }
  }

  /**
   * Clear the cache.
   */
  void clear() {
    std::scoped_lock lock(cacheMutex_);
    cache_[0].clear();
    cache_[1].clear();
    activeBucket_ = 0;
  }

private:
  uint64_t cacheSize_; ///< The currently configured maximum size for each one of the two buckets.
  mutable std::mutex cacheMutex_; ///< Mutex protecting the cache.
  std::array<std::unordered_map<K, V, Hasher, KeyEqual>, 2> cache_; ///< The cache buckets.
  uint64_t activeBucket_; ///< Index of the current bucket for insertion (0 or 1).
};

#endif // _BUCKETCACHE_H_