#ifndef _BUCKETCACHE_H_
#define _BUCKETCACHE_H_

#include <unordered_map>
#include <mutex>
#include <array>
#include <optional>

/**
 * A cache backed by two rotating unordered maps of K to V.
 * NOTE: This class is thread-safe.
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
   * @param autoFlipSecs If > 0, flip buckets when put() is called and this amount of
   * seconds has elapsed since the last bucket flip; Defaults to 0 (disabled).
   */
  explicit BucketCache(uint64_t cacheSize, int64_t autoFlipSecs = 0)
    : cacheSize_(cacheSize),
      activeBucket_(0),
      autoFlipSecs_(autoFlipSecs),
      lastFlipSecs_(now())
  {
  }

  /**
   * Save an entry in the cache, possibly causing a bucket clear and flip.
   * @param key The key to store.
   * @param value The value to store.
   */
  void put(const K& key, const V& value) {
    std::scoped_lock lock(cacheMutex_);
    checkFlip();
    auto& bucket = cache_[activeBucket_];
    bucket[key] = value;
    if (cacheSize_ > 0 && bucket.size() >= cacheSize_) {
      flip();
    }
  }

  /**
   * Get an entry from the cache.
   * @param key The key to look up.
   * @return The value associated with the key, or an empty std::optional if not found.
   */
  std::optional<V> get(const K& key) const {
    std::scoped_lock lock(cacheMutex_);
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
   * Remove an entry from the cache, if it can be found.
   * @param key Key of entry to remove.
   * @return `true` if an entry was found and removed, `false` otherwise.
   */
  bool erase(const K& key) {
    // Checks both buckets since put() doesn't check if the secondary bucket already has the key
    // before inserting it in the active bucket.
    std::scoped_lock lock(cacheMutex_);
    bool erased = false;
    for (int i = 0; i < 2; ++i) {
      const auto& bucket = cache_[i];
      auto it = bucket.find(key);
      if (it != bucket.end()) {
        bucket.erase(it);
        erased = true;
      }
    }
    return erased;
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

  /**
   * If autoFlipSecs_ > 0, check if it's time to flip.
   */
  void checkFlip() {
    if (autoFlipSecs_ > 0) {
      int64_t nowSecs = now();
      if (nowSecs - lastFlipSecs_ >= autoFlipSecs_) {
        lastFlipSecs_ = nowSecs;
        flip();
      }
    }
  }

  /**
   * Flip buckets.
   */
  void flip() {
    activeBucket_ = 1 - activeBucket_;
    cache_[activeBucket_].clear();
    lastFlipSecs_ = now();
  }

  /**
   * Helper for getting the current time secs since epoch.
   */
  int64_t now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
  }

  uint64_t cacheSize_; ///< The currently configured maximum size for each one of the two buckets.
  mutable std::mutex cacheMutex_; ///< Mutex protecting the cache.
  std::array<std::unordered_map<K, V, Hasher, KeyEqual>, 2> cache_; ///< The cache buckets.
  uint64_t activeBucket_; ///< Index of the current bucket for insertion (0 or 1).
  int64_t autoFlipSecs_; ///< If > 0, flip buckets when this amount of seconds has elapsed since the last bucket flip.
  int64_t lastFlipSecs_; ///< Secs since epoch the last flip was performed (default value set to now() at ctor).
};

#endif // _BUCKETCACHE_H_