/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEUNORDEREDMAP_H
#define SAFEUNORDEREDMAP_H

#include <unordered_map>
#include <utility>

#include "../../utils/safehash.h"

#include "safebase.h"

// TODO: somehow figure out a way to make loops work with this class (for (const auto& [key, value] : map) { ... })
/**
 * Safe wrapper for a `std::unordered_map`. Used to safely store an unordered map within a contract.
 * @tparam Key The map's key type.
 * @tparam T The map's value type.
 * @see SafeBase
 */
template <typename Key, typename T> class SafeUnorderedMap : public SafeBase {
  private:
    std::unordered_map<Key, T, SafeHash> value_; ///< Current ("original") value.
    std::unordered_map<Key, std::optional<T>, SafeHash> copy_; ///< Previous ("temporary") value. Stores changed keys only.

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param map The initial value. Defaults to an empty map.
     */
    explicit SafeUnorderedMap(
      DynamicContract* owner, const std::unordered_map<Key, T, SafeHash>& map = {}
    ) : SafeBase(owner), value_(map), copy_() {}

    /**
     * Empty constructor.
     * @param map The initial value. Defaults to an empty map.
     */
    explicit SafeUnorderedMap(const std::unordered_map<Key, T, SafeHash>& map = {})
      : SafeBase(nullptr), value_(map), copy_() {}

    /// Copy constructor. Copies only the CURRENT value.
    SafeUnorderedMap(const SafeUnorderedMap& other) : SafeBase(nullptr), value_(other.value_), copy_() {}

    /// Get the current value.
    std::unordered_map<Key, T, SafeHash> get() const { return this->value_; }

    /**
     * Get the number of values with the given key.
     * @param key The key of the values to count.
     * @return The number of values with the given key.
     */
    inline size_t count(const Key &key) const { return this->value_.count(key); }

    // TODO: find, begin() and end() return const iterator on purpose! we need SafeIterators to do this right (normal iterator doesn't have copy logic)

    /**
     * Find a given key (non-const).
     * @param key The key to find.
     * @return An iterator to the found key and its value.
     */
    const typename std::unordered_map<Key, T, SafeHash>::iterator find(const Key& key) {
      auto it = this->value_.find(key);
      if (it != this->value_.end()) this->copy_.try_emplace((*it).first, std::in_place, (*it).second);
      return it;
    }

    /**
     * Find a given key (const).
     * @param key The key to find.
     * @return An iterator to the found key and its value.
     */
    typename std::unordered_map<Key, T, SafeHash>::const_iterator find(const Key& key) const {
      return this->value_.find(key);
    }

    /**
     * Check if the map contains a given key.
     * @param key The key to check.
     * @return `true` if the unordered_map contains the given key, `false` otherwise.
     */
    inline bool contains(const Key &key) const { return this->value_.contains(key); }

    /// Get an iterator to the start of the original map value.
    inline const typename std::unordered_map<Key, T>::iterator begin() noexcept {
      // begin() points to *the* first element (if it exists), so a copy is required
      auto itValue = this->value_.find((*this->value_.begin()).first);
      if (itValue != this->value_.end()) {
        this->copy_.try_emplace((*itValue).first, std::in_place, (*itValue).second);
      }
      markAsUsed(); return this->value_.begin();
    }

    /// Get an iterator to the end of the original map value.
    inline const typename std::unordered_map<Key, T>::iterator end() noexcept {
      // end() points to *past* the last element (not *the* last one), so no copy is required
      markAsUsed(); return this->value_.end();
    }

    /// Get a const iterator to the start of the original map value.
    inline typename std::unordered_map<Key, T>::const_iterator cbegin() const noexcept { return this->value_.cbegin(); }

    /// Get a const iterator to the end of the original map value.
    inline typename std::unordered_map<Key, T>::const_iterator cend() const noexcept { return this->value_.cend(); }

    /**
     * Check if the map is empty (has no values).
     * @return `true` if map is empty, `false` otherwise.
     */
    inline bool empty() const noexcept { return this->value_.empty(); }

    /**
     * Get the size of the map.
     * @return The current size of the map.
     */
    inline size_t size() const noexcept { return this->value_.size(); }

    /// Clear the map.
    inline void clear() {
      for (const auto& [key, value] : this->value_) {
        // try_emplace will only insert if the key doesn't exist.
        this->copy_.try_emplace(key, std::in_place, value);
      }
      markAsUsed(); this->value_.clear();
    }

    /**
     * Insert a value into the map. Does nothing if the key already exists.
     * @param value The value to insert.
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    std::pair<typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool> insert(
      const typename std::unordered_map<Key, T, SafeHash>::value_type& value
    ) {
      auto ret = this->value_.insert(value);
      // Only register as changed if insert was successful.
      if (ret.second) {
        markAsUsed();
        // We must use try_emplace here because the key might already exist in copy_ (and we NEVER overwrite copy_).
        this->copy_.try_emplace(ret.first->first, std::nullopt);
      }
      return ret;
    }

    ///@{
    /**
     * Insert a value into the map, using move.
     * @param value The value to insert.
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    std::pair<typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool> insert(
      typename std::unordered_map<Key, T, SafeHash>::value_type&& value
    ) {
      auto ret = this->value_.insert(std::move(value));
      // Only register as changed if insert was successful.
      if (ret.second) {
        markAsUsed();
        // We must use try_emplace here because the key might already exist in copy_ (and we NEVER overwrite copy_).
        this->copy_.try_emplace((*ret.first).first, std::nullopt);
      }
      return ret;
    }

    template <typename P> requires std::is_same_v<P, std::pair<Key, T>>
    std::pair<typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool> insert(P&& value) {
      auto ret = this->value_.insert(std::move(value));
      // Only register as changed if insert was successful.
      if (ret.second) {
        markAsUsed();
        // We must use try_emplace here because the key might already exist in copy_ (and we NEVER overwrite copy_).
        this->copy_.try_emplace(ret.first->first, std::nullopt);
      }
      return ret;
    }
    ///@}

    /**
     * Insert a value into the map, using copy and a hint (the position before the insertion).
     * @param hint The hint to use.
     * @param value The value to insert.
     * @return An iterator to the inserted value.
     */
    typename std::unordered_map<Key, T, SafeHash>::const_iterator insert(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
      const typename std::unordered_map<Key, T, SafeHash>::value_type& value
    ) {
      auto ret = this->value_.insert(hint, value);
      // Only register as changed if insert was successful.
      if (ret != this->value_.cend()) {
        markAsUsed();
        // We must use try_emplace here because the key might already exist in copy_ (and we NEVER overwrite copy_).
        this->copy_.try_emplace(ret->first, std::nullopt);
      }
      return ret;
    }

    ///@{
    /**
     * Insert a value into the map, using move and a hint (the position before the insertion).
     * @param hint The hint to use.
     * @param value The value to insert.
     * @return An iterator to the inserted value.
     */
    typename std::unordered_map<Key, T, SafeHash>::const_iterator insert(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
      typename std::unordered_map<Key, T, SafeHash>::value_type&& value
    ) {
      auto ret = this->value_.insert(hint, std::move(value));
      // Only register as changed if insert was successful.
      if (ret != this->value_.cend()) {
        markAsUsed();
        // We must use try_emplace here because the key might already exist in copy_ (and we NEVER overwrite copy_).
        this->copy_.try_emplace(ret->first, std::nullopt);
      }
      return ret;
    }
    template <class P> typename std::unordered_map<Key, T, SafeHash>::const_iterator insert(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, P&& value
    ) {
      auto ret = this->value_.insert(hint, std::move(value));
      // Only register as changed if insert was successful.
      if (ret != this->value_.cend()) {
        markAsUsed();
        // We must use try_emplace here because the key might already exist in copy_ (and we NEVER overwrite copy_).
        this->copy_.try_emplace(ret->first, std::nullopt);
      }
      return ret;
    }
    ///@}

    /**
     * Insert a range of values into the map.
     * @tparam InputIt Any type of iterator.
     * @param first An iterator to the first value of the range.
     * @param last An iterator to the last value of the range.
     */
    template <class InputIt> void insert(InputIt first, InputIt last) {
      // On this insert, we copy everything because we cannot check the insert
      // return to see what keys were insertted.
      for (auto it = first; it != last; ++it) {
        auto valueIt = this->value_.find(it->first);
        if (valueIt != this->value_.end()) {
          this->copy_.try_emplace(it->first, std::in_place, valueIt->second);
        } else {
          this->copy_.try_emplace(it->first, std::nullopt);
        }
      }
      markAsUsed(); this->value_.insert(first, last);
    }

    /**
     * Insert a list of values into the map.
     * @param ilist The list of values to insert.
     */
    void insert(std::initializer_list<
      typename std::unordered_map<Key, T, SafeHash>::value_type
    > ilist) {
      for (const std::pair<Key, T>& item : ilist) {
        auto valueIt = this->value_.find(item.first);
        if (valueIt != this->value_.end()) {
          // Try to make a original copy of the value.
          this->copy_.try_emplace(item.first, std::in_place, valueIt->second);
        } else {
          // No value found, insert a empty optional.
          this->copy_.try_emplace(item.first, std::nullopt);
        }
      }
      markAsUsed(); this->value_.insert(ilist);
    }

    /**
     * Insert a value into the map, using move with a node handle.
     * @param nh The value to insert.
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    typename std::unordered_map<Key, T, SafeHash>::insert_return_type
    insert(typename std::unordered_map<Key, T, SafeHash>::node_type&& nh) {
      // Since node will be moved we can't rely on insert return as it'll be empty,
      // so we have to predict if insert will fail or not
      if (!this->value_.contains(nh.key())) { markAsUsed(); this->copy_.try_emplace(nh.key(), std::nullopt); }
      return this->value_.insert(std::move(nh));
    }

    /**
     * Insert a value into the map, using move with a node handle and a hint (the position before the insertion).
     * @param nh The value to insert.
     * @param hint The hint to use.
     * @return An iterator to the inserted value.
     */
    typename std::unordered_map<Key, T, SafeHash>::const_iterator insert(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
      typename std::unordered_map<Key, T, SafeHash>::node_type&& nh
    ) {
      // Since node will be moved we can't rely on insert return as it'll be empty,
      // so we have to predict if insert will fail or not
      if (!this->value_.contains(nh.key())) { markAsUsed(); this->copy_.try_emplace(nh.key(), std::nullopt); }
      return this->value_.insert(hint, std::move(nh));
    }

    /**
     * Insert a value into the map, or assign it if the key already exists.
     * @param k The key to insert.
     * @param obj The value to insert.
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    std::pair<typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool>
    insert_or_assign(const Key& k, const T& obj) {
      auto valueIt = this->value_.find(k);
      if (valueIt != this->value_.end()) {
        this->copy_.try_emplace(k, std::in_place, (*valueIt).second);
      } else {
        this->copy_.try_emplace(k, std::nullopt);
      }
      markAsUsed(); return this->value_.insert_or_assign(k, obj);
    }

    /**
     * Insert a value into the map, or assign it if the key already exists, using move.
     * @param k The key to insert.
     * @param obj The value to insert.
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    std::pair<typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool> insert_or_assign(Key&& k, T&& obj) {
      auto valueIt = this->value_.find(k);
      if (valueIt != this->value_.end()) {
        this->copy_.try_emplace(k, std::in_place, valueIt->second);
      } else {
        this->copy_.try_emplace(k, std::nullopt);
      }
      markAsUsed(); return this->value_.insert_or_assign(std::move(k), std::move(obj));
    }

    /**
     * Insert a value into the map, or assign it if the key already exists,
     * using a hint (the position before the insertion).
     * @param hint The hint to use.
     * @param k The key to insert.
     * @param obj The value to insert.
     * @return An iterator to the inserted value.
     */
    typename std::unordered_map<Key, T, SafeHash>::const_iterator insert_or_assign(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
      const Key& k, const T& obj
    ) {
      auto valueIt = this->value_.find(k);
      if (valueIt != this->value_.end()) {
        this->copy_.try_emplace(k, std::in_place, valueIt->second);
      } else {
        this->copy_.try_emplace(k, std::nullopt);
      }
      markAsUsed(); return this->value_.insert_or_assign(hint, k, obj);
    }

    /**
     * Insert a value into the map, or assign it if the key already exists,
     * using move and a hint (the position before the insertion).
     * @param hint The hint to use.
     * @param k The key to insert.
     * @param obj The value to insert.
     * @return An iterator to the inserted value.
     */
    typename std::unordered_map<Key, T, SafeHash>::const_iterator insert_or_assign(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
      Key&& k, T&& obj
    ) {
      auto valueIt = this->value_.find(k);
      if (valueIt != this->value_.end()) {
        this->copy_.try_emplace(k, std::in_place, valueIt->second);
      } else {
        this->copy_.try_emplace(k, std::nullopt);
      }
      markAsUsed(); return this->value_.insert_or_assign(hint, std::move(k), std::move(obj));
    }

    /**
     * Emplace a value into the map.
     * @param args The argument to build the value for insertion (not variadic! it's just one value).
     * @return A pair consisting of an iterator to the inserted value and a
     *         boolean indicating whether the insertion was successful.
     */
    template <typename... Args> std::pair<
      typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool
    > emplace(Args&&... args) {
      // emplace is the same as insert, it doesn`t replace the value if it already exists.
      // So as there is no "copy" is the
      auto ret = this->value_.emplace(std::forward<Args>(args)...);
      if (ret.second) {
        markAsUsed();
        // We must use try_emplace here because the key might already exist in copy_ (and we NEVER overwrite copy_).
        this->copy_.try_emplace(ret.first->first, std::nullopt);
      }
      markAsUsed(); return ret;
    }

    /**
     * Emplace a value into the map, using a hint (the position before the insertion).
     * @param hint The hint to use.
     * @param args The arguments to build the value for insertion.
     * @return An iterator to the inserted value.
     */
    template <typename... Args> typename std::unordered_map<Key, T, SafeHash>::const_iterator emplace_hint(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, Args&&... args
    ) {
      auto ret = this->value_.emplace_hint(hint, std::forward<Args>(args)...);
      if (ret != this->value_.cend()) {
        markAsUsed();
        // We must use try_emplace here because the key might already exist in copy_ (and we NEVER overwrite copy_).
        this->copy_.try_emplace(ret->first, std::nullopt);
      }
      return ret;
    }

    /**
     * Try emplacing a value into the map, do nothing if key already exists.
     * @param key The key to emplace.
     * @param args The argument to build the value for emplace (not variadic! it's just one value).
     * @return A pair consisting of an iterator to the emplaced value and a
     *         boolean indicating whether the emplace was successful.
     */
    template <typename... Args> std::pair<typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool>
    try_emplace(const Key& key, Args&&... args) {
      auto ret = this->value_.try_emplace(key, std::forward<Args>(args)...);
      if (ret.second) {
        markAsUsed();
        // We must use try_emplace here because the key might already exist in copy_ (and we NEVER overwrite copy_).
        this->copy_.try_emplace(key, std::nullopt);
      }
      return ret;
    }

    /**
     * Try emplacing a value into the map, using move/forward on the key.
     * @param key The key to emplace.
     * @param args The argument to build the value for emplace (not variadic! it's just one value).
     * @return A pair consisting of an iterator to the emplaced value and a
     *         boolean indicating whether the emplace was successful.
     */
    template <typename... Args> std::pair<typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool>
    try_emplace(Key&& key, Args&&... args) {
      auto ret = this->value_.try_emplace(std::move(key), std::forward<Args>(args)...);
      if (ret.second) {
        markAsUsed();
        // We must use try_emplace here because the key might already exist in copy_ (and we NEVER overwrite copy_).
        this->copy_.try_emplace(key, std::nullopt);
      }
      return ret;
    }

    /**
     * Try emplacing a value into the map, using a hint.
     * @param hint The hint to use.
     * @param key The key to emplace.
     * @param args The argument to build the value for emplace (not variadic! it's just one value).
     * @return An iterator to the emplaced value.
     */
    template <typename... Args> typename std::unordered_map<Key, T, SafeHash>::const_iterator
    try_emplace(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, const Key& key, Args&&... args) {
      // Only copy the value if key already exists (this overload doesn't return
      // a pair so we don't know if insertion was successful)
      if (!this->value_.contains(key)) { markAsUsed(); this->copy_.try_emplace(key, std::nullopt); }
      return this->value_.try_emplace(hint, key, std::forward<Args>(args)...);
    }

    /**
     * Try emplacing a value into the map, using a hint and move/forward.
     * @param hint The hint to use.
     * @param key The key to emplace.
     * @param args The argument to build the value for emplace (not variadic! it's just one value).
     * @return An iterator to the emplaced value.
     */
    template <typename... Args> typename std::unordered_map<Key, T, SafeHash>::const_iterator
    try_emplace(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, Key&& key, Args&&... args) {
      // Only copy the value if key already exists (this overload doesn't return
      // a pair so we don't know if insertion was successful)
      if (!this->value_.contains(key)) { markAsUsed(); this->copy_.try_emplace(key, std::nullopt); }
      return this->value_.try_emplace(hint, std::move(key), std::forward<Args>(args)...);
    }

    /**
     * Erase a value from the map.
     * @param pos The position of the value to erase.
     * @return An iterator to the next value.
     */
    typename std::unordered_map<Key, T, SafeHash>::const_iterator erase(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator pos
    ) {
      auto itValue = this->value_.find((*pos).first);
      if (itValue != this->value_.end()) {
        this->copy_.try_emplace((*itValue).first, std::in_place, (*itValue).second);
      } else {
        this->copy_.try_emplace((*itValue).first, std::nullopt);
      }
      markAsUsed(); return this->value_.erase(pos);
    }

    /**
     * Erase a range of values from the map.
     * @param first The first position to erase.
     * @param last The last position to erase.
     * @return An iterator to the next value.
     */
    typename std::unordered_map<Key, T, SafeHash>::const_iterator erase(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator first,
      typename std::unordered_map<Key, T, SafeHash>::const_iterator last
    ) {
      for (auto it = first; it != last; ++it) {
        auto itValue = this->value_.find((*it).first);
        if (itValue != this->value_.end()) {
          this->copy_.try_emplace((*itValue).first, std::in_place, (*itValue).second);
        } else {
          this->copy_.try_emplace((*itValue).first, std::nullopt);
        }
      }
      markAsUsed(); return this->value_.erase(first, last);
    }

    /**
     * Erase a value from the map, using a key.
     * @param key The key of the value to erase.
     * @return The number of values erased.
     */
    typename std::unordered_map<Key, T, SafeHash>::size_type erase(const Key& key) {
      auto itValue = this->value_.find(key);
      if (itValue != this->value_.end()) {
        this->copy_.try_emplace((*itValue).first, std::in_place, (*itValue).second);
      } else {
        this->copy_.try_emplace((*itValue).first, std::nullopt);
      }
      markAsUsed(); return this->value_.erase(key);
    }

    /**
     * Extract a node (key+value) from the map, using an iterator.
     * @param pos The position of the node to extract.
     * @return The extracted node handle.
     */
    inline typename std::unordered_map<Key, T, SafeHash>::node_type extract(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator pos
    ) {
      auto itValue = this->value_.find((*pos).first);
      if (itValue != this->value_.cend()) {
        this->copy_.try_emplace((*itValue).first, std::in_place, (*itValue).second);
      } else {
        this->copy_.try_emplace((*itValue).first, std::nullopt);
      }
      markAsUsed(); return this->value_.extract(pos);
    }

    /**
     * Extract a node (key+value) from the map, using a key.
     * @param k The key of the node to extract.
     * @return The extracted node handle.
     */
    inline typename std::unordered_map<Key, T, SafeHash>::node_type extract(const Key& k) {
      auto itValue = this->value_.find(k);
      if (itValue != this->value_.cend()) {
        this->copy_.try_emplace((*itValue).first, std::in_place, (*itValue).second);
      } else {
        this->copy_.try_emplace((*itValue).first, std::nullopt);
      }
      markAsUsed(); return this->value_.extract(k);
    }

    ///@{
    /**
     * Get the value with the given key.
     * @param key The key to get the value from.
     * @return A reference to the value within the key.
     * @throws std::out_of_range if key does not exist in the map.
     */
    inline T& at(const Key& key) {
      auto valueIt = this->value_.find(key);
      if (valueIt != this->value_.end()) {
        this->copy_.try_emplace(key, std::in_place, (*valueIt).second);
      } else {
        this->copy_.try_emplace(key, std::nullopt);
      }
      markAsUsed(); return this->value_.at(key);
    }
    inline const T& at(const Key& key) const { return this->value_.at(key); }
    ///@}

    ///@{
    /** Subscript/indexing operator. Creates the key if it doesn't exist. */
    T& operator[](const Key& key) {
      auto valueIt = this->value_.find(key);
      if (valueIt != this->value_.end()) {
        this->copy_.try_emplace(key, std::in_place, valueIt->second);
      } else {
        this->copy_.try_emplace(key, std::nullopt);
      }
      markAsUsed(); return this->value_[key];
    }
    T& operator[](Key&& key) {
      auto valueIt = this->value_.find(key);
      if (valueIt != this->value_.end()) {
        this->copy_.try_emplace(key, std::in_place, valueIt->second);
      } else {
        this->copy_.try_emplace(key, std::nullopt);
      }
      markAsUsed(); return this->value_[std::move(key)];
    }
    ///@}

    ///@{
    /** Equality operator. Checks only the CURRENT value. */
    inline bool operator==(const std::unordered_map<Key, T, SafeHash>& other) { return this->value_ == other; }
    inline bool operator==(const SafeUnorderedMap& other) { return this->value_ == other.get(); }
    ///@}

    /// Commit the value.
    void commit() override { this->copy_.clear(); this->registered_ = false; }

    /// Revert the value.
    void revert() override {
      for (const auto& [key, value] : this->copy_) {
        if (value == std::nullopt) {
          this->value_.erase(key);
        } else {
          this->value_.insert_or_assign(key, value.value());
        }
      }
      this->copy_.clear(); this->registered_ = false;
    }
};

#endif // SAFEUNORDEREDMAP_H
