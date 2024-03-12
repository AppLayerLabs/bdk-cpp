/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEUNORDEREDMAP_H
#define SAFEUNORDEREDMAP_H

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "../../utils/safehash.h"
#include "safebase.h"

/**
 * Safe wrapper for a `std::unordered_map`. Used to safely store an unordered map within a contract.
 * @tparam Key The map's key type.
 * @tparam T The map's value type.
 * @see SafeBase
 */
template <typename Key, typename T> class SafeUnorderedMap : public SafeBase {
  private:
    std::unordered_map<Key, T, SafeHash> map_; ///< Value.
    mutable std::unique_ptr<std::unordered_map<Key, T, SafeHash>> mapPtr_; ///< Pointer to the value.
    mutable std::unique_ptr<std::unordered_set<Key, SafeHash>> erasedKeys_; ///< Pointer to the set of erased keys in map_ (not mapPtr_).
    mutable uint64_t size_; ///< Cache of this container's element count (valid only if dirtySize_ == false)
    mutable bool dirtySize_; ///< True if size_ has to be be recomputed

    /// Check if pointers are initialized (and initialize them if not).
    inline void check() const override {
      if (mapPtr_ == nullptr) mapPtr_ = std::make_unique<std::unordered_map<Key, T, SafeHash>>();
      if (erasedKeys_ == nullptr) erasedKeys_ = std::make_unique<std::unordered_set<Key, SafeHash>>();
    }

    /**
     * Check if a key exists and only copy if it truly does.
     * @param key The key to check.
     */
    inline void checkKeyAndCopy(const Key& key) const {
      check();
      auto itP = mapPtr_->find(key);
      if (itP == mapPtr_->end()) {
        auto itM = map_.find(key);
        if (itM != map_.end()) {
          auto itD = erasedKeys_->find(key);
          if (itD == erasedKeys_->end()) (*mapPtr_)[key] = itM->second;
        }
      }
    }

    /**
     * Check if a key exists and create a new one if it doesn't.
     * @param key The key to check.
     */
    inline void checkKeyAndCreate(const Key& key) const {
      check();
      auto itP = mapPtr_->find(key);
      if (itP == mapPtr_->end()) {
        auto itM = map_.find(key);
        if (itM == map_.end()) {
          (*mapPtr_)[key] = T();
          ++size_;
        } else {
          auto itD = erasedKeys_->find(key);
          if (itD == erasedKeys_->end()) {
            (*mapPtr_)[key] = itM->second;
          } else {
            (*mapPtr_)[key] = T();
            ++size_;
          }
        }
      }
    }

    /**
     * Check if a key is committed to the map.
     * @param key The key to check.
     * @return `true` if  `key` in present in map_ and absent from erasedKeys_,
     *         false otherwise.
     */
    inline bool hasCommittedKey(const Key& key) const {
      return map_.find(key) != map_.end() && erasedKeys_->find(key) == erasedKeys_->end();
    }

    /**
     * Check if a key exists, throw if it doesn't.
     * @param key The key to check.
     * @throw DynamicException if key doesn't exist.
     */
    inline void checkKeyAndThrow(const Key& key) const {
      check();
      auto itP = mapPtr_->find(key);
      if (itP == mapPtr_->end()) {
        auto itM = map_.find(key);
        if (itM == map_.end()) {
          throw DynamicException("Key not found");
        } else {
          auto itD = erasedKeys_->find(key);
          if (itD == erasedKeys_->end()) {
            (*mapPtr_)[key] = itM->second;
          } else {
            throw DynamicException("Key not found");
          }
        }
      }
    }

  public:
    using value_type = typename std::unordered_map<Key, T, SafeHash>::value_type;
    using size_type = typename std::unordered_map<Key, T, SafeHash>::size_type;

    class iterator;

    /**
     * SafeUnorderedMap const iterator.
     * Iterates over both committed and uncommitted elements.
     * Using an iterator after the underlying container(s) rehash can result in undefined behavior.
     */
    class const_iterator {
      private:
        friend class SafeUnorderedMap<Key, T>;
        const SafeUnorderedMap<Key, T>* container_;
        mutable typename std::unordered_map<Key, T, SafeHash>::const_iterator it_;
        mutable bool inMapPtr_;

        void skip() const {
          container_->check();
          if (inMapPtr_ && it_ == container_->mapPtr_->end()) {
            it_ = container_->map_.begin();
            inMapPtr_ = false;
          }
          while (!inMapPtr_) {
            if (it_ == container_->map_.end()) break;
            if (container_->erasedKeys_->find(it_->first) == container_->erasedKeys_->end() &&
                container_->mapPtr_->find(it_->first) == container_->mapPtr_->end())
            {
              return;
            }
            ++it_;
          }
        }

      public:
        const_iterator(const SafeUnorderedMap<Key, T>* container,
                       typename std::unordered_map<Key, T, SafeHash>::const_iterator it,
                       bool inMapPtr)
          : container_(container), it_(it), inMapPtr_(inMapPtr)
          { skip(); }

        iterator as_iterator() const { return iterator(container_, it_, inMapPtr_); }

        const_iterator& operator++() { ++it_; skip(); return *this; }
        const_iterator operator++(int) { const_iterator tmp = *this; ++(*this); return tmp; }
        const std::pair<const Key, T>& operator*() const { return *it_; }
        const std::pair<const Key, T>* operator->() const { return &(*it_); }
        bool operator!=(const const_iterator& other) const { return !(*this == other); }
        bool operator==(const const_iterator& other) const { return it_ == other.it_ && inMapPtr_ == other.inMapPtr_; }

        bool operator==(const iterator& other) const {
          auto rhs = other.as_const_iterator();
          return it_ == rhs.it_ && inMapPtr_ == rhs.inMapPtr_;
        }
        bool operator!=(const iterator& other) const { return !(*this == other); }
    };

    /**
     * SafeUnorderedMap iterator.
     * Iterates over both committed and uncommitted elements.
     * Using an iterator after the underlying container(s) rehash can result in undefined behavior.
     */
    class iterator {
      private:
        friend class SafeUnorderedMap<Key, T>;
        SafeUnorderedMap<Key, T>* container_;
        mutable typename std::unordered_map<Key, T, SafeHash>::iterator it_;
        mutable bool inMapPtr_;
        mutable std::optional<typename std::unordered_map<Key, T, SafeHash>::iterator> upgradeIt_;

        void skip() {
          container_->check();
          if (inMapPtr_ && it_ == container_->mapPtr_->end()) {
            it_ = container_->map_.begin();
            inMapPtr_ = false;
          }
          while (!inMapPtr_) {
            if (it_ == container_->map_.end()) break;
            if (container_->erasedKeys_->find(it_->first) == container_->erasedKeys_->end() &&
                container_->mapPtr_->find(it_->first) == container_->mapPtr_->end())
            {
              return;
            }
            ++it_;
          }
        }

      public:
        iterator(SafeUnorderedMap<Key, T>* container,
                 typename std::unordered_map<Key, T, SafeHash>::iterator it,
                 bool inMapPtr)
          : container_(container), it_(it), inMapPtr_(inMapPtr)
          { skip(); }

        const_iterator as_const_iterator() const { return const_iterator(container_, it_, inMapPtr_); }

        iterator& operator++() { upgradeIt_.reset(); ++it_; skip(); return *this; }
        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
        std::pair<const Key, T>& operator*() const {
          container_->markAsUsed();
          if (!inMapPtr_) {
            if (!upgradeIt_.has_value()) {
              upgradeIt_ = std::make_optional<typename std::unordered_map<Key, T, SafeHash>::iterator>
                (container_->mapPtr_->insert_or_assign(it_->first, it_->second).first);
            }
            return *upgradeIt_.value();
          }
          return *it_;
        }
        std::pair<const Key, T>* operator->() const { return std::addressof(operator*()); }
        bool operator!=(const iterator& other) const { return !(*this == other); }
        bool operator==(const iterator& other) const { return it_ == other.it_ && inMapPtr_ == other.inMapPtr_; }
    };

    /**
     * Get a const iterator to the start of the map.
     * @return A const_iterator to the start of the map.
     */
    const_iterator cbegin() const noexcept { return begin(); }

    /**
     * Get a const iterator to the end of the map.
     * @return A const_iterator to the end of the map.
     */
    const_iterator cend() const noexcept { return end(); }

    /**
     * Get an iterator to the start of the map.
     * @return An iterator to the start of the map.
     */
    iterator begin() noexcept {
      if (mapPtr_ == nullptr) return iterator(this, map_.begin(), false);
      return iterator(this, mapPtr_->begin(), true);
    }

    /**
     * Get an iterator to the end of the map.
     * @return An iterator to the end of the map.
     */
    iterator end() noexcept { return iterator(this, map_.end(), false); }

    /**
     * Get a const iterator to the start of the map.
     * @return A const_iterator to the start of the map.
     */
    const_iterator begin() const noexcept {
      if (mapPtr_ == nullptr) return const_iterator(this, map_.begin(), false);
      return const_iterator(this, mapPtr_->begin(), true);
    }

    /**
     * Get a const iterator to the end of the map.
     * @return A const_iterator to the end of the map.
     */
    const_iterator end() const noexcept { return const_iterator(this, map_.end(), false); }

    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param map The initial value. Defaults to an empty map.
     */
    SafeUnorderedMap(
      DynamicContract* owner, const std::unordered_map<Key, T, SafeHash>& map = {}
      ) : SafeBase(owner), map_(map), size_(map.size()), dirtySize_(false) {}

    /**
     * Empty constructor.
     * @param map The initial value. Defaults to an empty map.
     */
    explicit SafeUnorderedMap(const std::unordered_map<Key, T, SafeHash>& map = {})
      : SafeBase(nullptr), mapPtr_(std::make_unique<std::unordered_map<Key, T, SafeHash>>(map)), size_(map.size()), dirtySize_(false) {}

    /// Copy constructor.
    SafeUnorderedMap(const SafeUnorderedMap& other) : SafeBase(nullptr) {
      other.check();
      map_ = other.map_;
      mapPtr_ = std::make_unique<std::unordered_map<Key, T, SafeHash>>(*other.mapPtr_);
      erasedKeys_ = std::make_unique<std::unordered_set<Key, SafeHash>>(*other.erasedKeys_);
      size_ = other.size_;
      dirtySize_ = other.dirtySize_;
    }

    /// Commit the value. Updates the values from the pointers, nullifies them and unregisters the variable.
    void commit() override {
      check();
      for (const auto& key : (*erasedKeys_)) map_.erase(key);
      map_.merge(*mapPtr_);
      for (const auto &[key, value] : (*mapPtr_)) map_[key] = value;
      revert();
    }

    /// Revert the value. Nullifies the pointers and unregisters the variable.
    void revert() const override {
      mapPtr_ = nullptr;
      erasedKeys_ = nullptr;
      dirtySize_ = false;
      size_ = map_.size();
      registered_ = false;
    }

    /**
     * Reserve space in this SafeUnorderedMap.
     * @param n Minimum capacity of the underlying unordered_map that tracks
     *        changes (as opposed to the one that tracks committed values).
     */
    inline void reserve(size_t n) { check(); mapPtr_->reserve(n); }

    /**
     * Check if the map contains a given key.
     * @param key The key to check.
     * @return `true` if the unordered_map contains the given key, `false` otherwise.
     */
    inline bool contains(const Key &key) const { return find(key) != cend(); }

    /**
     * Get the number of values with the given key.
     * @param key The key of the values to count.
     * @return The number of values with the given key.
     */
    inline size_t count(const Key &key) const { return contains(key) ? 1 : 0; }

    /**
     * Get the size of the map.
     * @return The size of the map.
     */
    inline size_t size() const noexcept {
      check();
      if (dirtySize_) {
        size_ = mapPtr_->size();
        for (const auto& [key, value] : map_) {
          if (mapPtr_->find(key) == mapPtr_->end() && erasedKeys_->find(key) == erasedKeys_->end()) { ++size_; }
        }
        dirtySize_ = false;
      }
      return size_;
    }

    /**
     * Check if the map is empty (has no values).
     * @return `true` if map is empty, `false` otherwise.
     */
    inline bool empty() const noexcept { return size() == 0; }

    ///@{
    /**
     * Find a given key.
     * @param key The key to find.
     * @return An iterator to the found key and its value.
     */
    typename SafeUnorderedMap<Key, T>::iterator find(const Key& key) {
      check();
      auto mapPtrIt = mapPtr_->find(key);
      if (mapPtrIt != mapPtr_->end()) {
        return iterator(this, mapPtrIt, true);
      }
      auto mapIt = map_.find(key);
      if (mapIt != map_.end()) {
        if (erasedKeys_->find(key) == erasedKeys_->end()) {
          return iterator(this, mapIt, false);
        }
      }
      return end();
    }
    typename SafeUnorderedMap<Key, T>::const_iterator find(const Key& key) const {
      check();
      auto mapPtrIt = mapPtr_->find(key);
      if (mapPtrIt != mapPtr_->end()) {
        return const_iterator(this, mapPtrIt, true);
      }
      auto mapIt = map_.find(key);
      if (mapIt != map_.end()) {
        if (erasedKeys_->find(key) == erasedKeys_->end()) {
          return const_iterator(this, mapIt, false);
        }
      }
      return end();
    }
    ///@}

    /**
     * Insert a value into the map.
     * @param value The value to insert.
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    std::pair<typename SafeUnorderedMap<Key, T>::iterator, bool> insert(
      const typename SafeUnorderedMap<Key, T>::value_type& value
    ) {
      check(); markAsUsed();
      auto r = mapPtr_->insert(value);
      if (!dirtySize_ && r.second && !hasCommittedKey(value.first)) ++size_;
      return {iterator(this, r.first, true), r.second};
    }

    /**
     * Insert a value into the map, using move.
     * @param value The value to insert.
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    std::pair<typename SafeUnorderedMap<Key, T>::iterator, bool> insert(
      const typename SafeUnorderedMap<Key, T>::value_type&& value
    ) {
      check(); markAsUsed();
      auto r = mapPtr_->insert(std::move(value));
      if (!dirtySize_ && r.second && !hasCommittedKey(value.first)) ++size_;
      return {iterator(this, r.first, true), r.second};
    }

    /**
     * Insert a value into the map, using copy and a hint (the position before the insertion).
     * @param hint The hint to use.
     * @param value The value to insert.
     * @return An iterator to the inserted value.
     */
    typename SafeUnorderedMap<Key, T>::iterator insert(
      typename SafeUnorderedMap<Key, T>::const_iterator hint,
      const typename SafeUnorderedMap<Key, T>::value_type& value
    ) {
      check(); markAsUsed(); dirtySize_ = true;
      auto r = mapPtr_->insert(hint.it_, value);
      return iterator(this, r, true);
    }

    /**
     * Insert a value into the map, using move and a hint (the position before the insertion).
     * @param hint The hint to use.
     * @param value The value to insert.
     * @return An iterator to the inserted value.
     */
    typename SafeUnorderedMap<Key, T>::iterator insert(
      typename SafeUnorderedMap<Key, T>::const_iterator hint,
      typename SafeUnorderedMap<Key, T>::value_type&& value
    ) {
      check(); markAsUsed(); dirtySize_ = true;
      auto r = mapPtr_->insert(hint.it_, std::move(value));
      return iterator(this, r, true);
    }

    /**
     * Insert a range of values into the map.
     * @tparam InputIt Any type of iterator.
     * @param first An iterator to the first value of the range.
     * @param last An iterator to the last value of the range.
     */
    template <class InputIt> void insert(InputIt first, InputIt last) {
      check(); markAsUsed(); dirtySize_ = true; mapPtr_->insert(first, last);
    }

    /**
     * Insert a list of values into the map.
     * @param ilist The list of values to insert.
     */
    void insert(
      std::initializer_list<typename SafeUnorderedMap<Key, T>::value_type> ilist
    ) {
      check(); markAsUsed(); dirtySize_ = true; mapPtr_->insert(ilist);
    }

    /**
     * Insert a value into the map, or assign it if the key already exists.
     * @param k The key to insert.
     * @param obj The value to insert.
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    std::pair<typename SafeUnorderedMap<Key, T>::iterator, bool> insert_or_assign(
      const Key& k, const T& obj
    ) {
      check(); markAsUsed();
      auto r = mapPtr_->insert_or_assign(k, obj);
      if (!dirtySize_ && r.second && !hasCommittedKey(k)) ++size_;
      return {iterator(this, r.first, true), r.second};
    }

    /**
     * Insert a value into the map, or assign it if the key already exists, using move.
     * @param k The key to insert.
     * @param obj The value to insert.
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    std::pair<typename SafeUnorderedMap<Key, T>::iterator, bool> insert_or_assign(
      Key&& k, T&& obj
    ) {
      check(); markAsUsed();
      auto r = mapPtr_->insert_or_assign(std::move(k), std::move(obj));
      if (!dirtySize_ && r.second && !hasCommittedKey(k)) ++size_;
      return {iterator(this, r.first, true), r.second};
    }

    /**
     * Insert a value into the map, or assign it if the key already exists,
     * using a hint (the position before the insertion).
     * @param hint The hint to use.
     * @param k The key to insert.
     * @param obj The value to insert.
     * @return An iterator to the inserted value.
     */
    typename SafeUnorderedMap<Key, T>::iterator insert_or_assign(
      typename SafeUnorderedMap<Key, T>::const_iterator hint,
      const Key& k, const T& obj
    ) {
      check(); markAsUsed(); dirtySize_ = true;
      auto r = mapPtr_->insert_or_assign(hint.it_, k, obj);
      return iterator(this, r, true);
    }

    /**
     * Insert a value into the map, or assign it if the key already exists,
     * using move and a hint (the position before the insertion).
     * @param hint The hint to use.
     * @param k The key to insert.
     * @param obj The value to insert.
     * @return An iterator to the inserted value.
     */
    typename SafeUnorderedMap<Key, T>::iterator insert_or_assign(
      typename SafeUnorderedMap<Key, T>::const_iterator hint,
      Key&& k, T&& obj
    ) {
      check(); markAsUsed(); dirtySize_ = true;
      auto r = mapPtr_->insert_or_assign(hint.it_, std::move(k), std::move(obj));
      return iterator(this, r, true);
    }

    /**
     * Emplace a value into the map.
     * @param args The arguments to build the value for insertion.
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    template <typename... Args> std::pair<
      typename SafeUnorderedMap<Key, T>::iterator, bool
    > emplace(Args&&... args) {
      check(); markAsUsed(); dirtySize_ = true;
      auto r = mapPtr_->emplace(std::forward<Args>(args)...);
      return {iterator(this, r.first, true), r.second};
    }

    /**
     * Emplace a value into the map, using a hint (the position before the insertion).
     * @param hint The hint to use.
     * @param args The arguments to build the value for insertion.
     * @return An iterator to the inserted value.
     */
    template <typename... Args> typename SafeUnorderedMap<Key, T>::iterator emplace_hint(
      typename SafeUnorderedMap<Key, T>::const_iterator hint,
      Args&& ...args
    ) {
      check(); markAsUsed(); dirtySize_ = true;
      auto r = mapPtr_->emplace_hint(hint.it_, std::forward<Args>(args)...);
      return iterator(this, r, true);
    }

    /**
     * Erase a value from the map.
     * @param pos The position of the value to erase.
     * @return An iterator to the next value.
     */
    typename SafeUnorderedMap<Key, T>::iterator erase(
      typename SafeUnorderedMap<Key, T>::iterator pos
    ) {
      check();
      markAsUsed();
      erasedKeys_->insert(pos.it_->first);
      if (pos.inMapPtr_) {
        pos.it_ = mapPtr_->erase(pos.it_);
      } else {
        ++pos.it_;
      }
      pos.skip();
      --size_;
      return pos;
    }

    /**
     * Erase a value from the map, using a const_iterator.
     * @param pos The position of the value to erase.
     * @return An iterator to the next value.
     */
    typename SafeUnorderedMap<Key, T>::iterator erase(
      typename SafeUnorderedMap<Key, T>::const_iterator pos
    ) {
      check();
      markAsUsed();
      erasedKeys_->insert(pos.it_->first);
      if (pos.inMapPtr_) {
        pos.it_ = mapPtr_->erase(pos.it_);
      } else {
        ++pos.it_;
      }
      pos.skip();
      --size_;
      return pos;
    }

    /**
     * Erase a value from the map, using a key.
     * @param key The key of the value to erase.
     * @return The number of values erased.
     */
    typename SafeUnorderedMap<Key, T>::size_type erase(const Key& key) {
      auto it = find(key);
      if (it == end()) return 0;
      erase(it);
      return 1;
    }

    /**
     * Erase a value from the map, using a key and move/forward.
     * @param key The key of the value to erase.
     * @return The number of values erased.
     */
    template <class K> typename SafeUnorderedMap<Key, T>::size_type erase(K&& key) {
      bool deleted = false;
      check();
      auto mapPtrIt = mapPtr_->find(std::forward<K>(key));
      if (mapPtrIt != mapPtr_->end()) {
        mapPtr_->erase(std::forward<K>(key));
        deleted = true;
      } else {
        auto mapIt = map_.find(std::forward<K>(key));
        if (mapIt != map_.end()) {
          if (erasedKeys_->find(std::forward<K>(key)) == erasedKeys_->end()) {
            deleted = true;
          }
        }
      }
      if (deleted) {
        markAsUsed();
        erasedKeys_->insert(std::forward<K>(key));
        --size_;
        return 1;
      }
      return 0;
    }

    ///@{
    /**
     * Get the value with the given key.
     * @param key The key to get the value from.
     * @return A reference to the value within the key.
     */
    inline T& at(const Key& key) { checkKeyAndThrow(key); markAsUsed(); return (*mapPtr_)[key]; }
    inline const T& at(const Key& key) const { checkKeyAndThrow(key); return (*mapPtr_)[key]; }
    ///@}

    ///@{
    /** Subscript/indexing operator. Creates the key if it doesn't exist. */
    T& operator[](const Key& key) { checkKeyAndCreate(key); markAsUsed(); return (*mapPtr_)[key]; }
    T& operator[](Key&& key) { checkKeyAndCreate(key); markAsUsed(); return (*mapPtr_)[key]; }
    ///@}

    /**
     * Assignment operator.
     * @param other The SafeUnorderedMap whose current, reversible value
     *        will be assigned to this object's current, reversible value.
     * @return A reference to this SafeUnorderedMap.
     */
    SafeUnorderedMap& operator=(const SafeUnorderedMap& other) {
      if (this != &other) {
        markAsUsed();
        other.check();
        // Fold all changes proposed to map_ by operator=(other) into mapPtr_ and erasedKeys_
        mapPtr_ = std::make_unique<std::unordered_map<Key, T, SafeHash>>(other.map_);
        for (const auto& key : *other.erasedKeys_) mapPtr_->erase(key);
        for (const auto& [key, value] : *other.mapPtr_) (*mapPtr_)[key] = value;
        erasedKeys_ = std::make_unique<std::unordered_set<Key, SafeHash>>(*other.erasedKeys_);
        dirtySize_ = true;
      }
      return *this;
    }
};

#endif // SAFEUNORDEREDMAP_H
