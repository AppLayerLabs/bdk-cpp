/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEUNORDEREDMAP_H
#define SAFEUNORDEREDMAP_H

#include <memory>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

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
    std::unordered_map<Key, std::unique_ptr<T>, SafeHash>> copy_; ///< Previous ("temporary") value. Stores changed keys only.

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param map The initial value. Defaults to an empty map.
     */
    SafeUnorderedMap(
      DynamicContract* owner, const std::unordered_map<Key, T, SafeHash>& map = {}
    ) : SafeBase(owner), value_(map), copy_(map) {}

    /**
     * Empty constructor.
     * @param map The initial value. Defaults to an empty map.
     */
    explicit SafeUnorderedMap(const std::unordered_map<Key, T, SafeHash>& map = {})
      : SafeBase(nullptr), value_(map), copy_(map) {}

    /// Copy constructor.
    SafeUnorderedMap(const SafeUnorderedMap& other) : SafeBase(nullptr), value_(other.value_), copy_(other.value_) {}

    /**
     * Get the number of values with the given key.
     * @param key The key of the values to count.
     * @return The number of values with the given key.
     */
    inline size_t count(const Key &key) const { return this->value_.count(key); }

    /**
     * Find a given key.
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
    inline std::unordered_map<Key, T>::const_iterator cbegin() const noexcept { return this->value_.cbegin(); }

    /// Get an iterator to the end of the original map value.
    inline std::unordered_map<Key, T>::const_iterator cend() const noexcept { return this->value_.cend(); }

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
      for (std::pair<Key, T> val : this->value_) {
        if (!this->copy_.contains(val.first)) this->copy_[val.first] = std::make_unique<T>(val.second);
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
      if (!this->value_.contains(value.first) && !this->copy_.contains(value.first)) {
        this->copy_[value.first] = nullptr;
      }
      markAsUsed(); return this->value_.insert(value);
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
      if (!this->value_.contains(value.first) && !this->copy_.contains(value.first)) {
        this->copy_[value.first] = nullptr;
      }
      markAsUsed(); return this->value_.insert(std::move(value));
    }
    template <typename P> requires std::is_same_v<P, std::pair<Key, T>>
    std::pair<typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool> insert(P&& value) {
      if (!this->value_.contains(value.first) && !this->copy_.contains(value.first)) {
        this->copy_[value.first] = nullptr;
      }
      markAsUsed(); return this->value_.insert(std::move(value));
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
      if (!this->value_.contains(value.first) && !this->copy_.contains(value.first)) {
        this->copy_[value.first] = nullptr;
      }
      copyKeyIfNotChanged(value.first, true); markAsUsed(); return this->value_.insert(hint, value);
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
      if (!this->value_.contains(value.first) && !this->copy_.contains(value.first)) {
        this->copy_[value.first] = nullptr;
      }
      markAsUsed(); return this->value_.insert(hint, std::move(value));
    }
    template <class P> typename std::unordered_map<Key, T, SafeHash>::const_iterator insert(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, P&& value
    ) {
      if (!this->value_.contains(value.first) && !this->copy_.contains(value.first)) {
        this->copy_[value.first] = nullptr;
      }
      markAsUsed(); return this->value_.insert(hint, std::move(value));
    }
    ///@}

    /**
     * Insert a range of values into the map.
     * @tparam InputIt Any type of iterator.
     * @param first An iterator to the first value of the range.
     * @param last An iterator to the last value of the range.
     */
    template <class InputIt> void insert(InputIt first, InputIt last) {
      for (auto it = first; it < last; it++) {
        if (!this->value_.contains((*it).first) && !this->copy_.contains((*it).first)) {
          this->copy_[(*it).first] = nullptr;
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
      for (std::pair<Key, T> item : ilist) {
        if (!this->value_.contains(item.first) && !this->copy_.contains(item.first)) {
          this->copy_[item.first] = nullptr;
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
      if (!this->value_.contains(nh.key()) && !this->copy_.contains(nh.key())) {
        this->copy_[nh.key()] = nullptr;
      }
      markAsUsed(); return this->value_.insert(std::move(nh));
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
      if (!this->value_.contains(nh.key()) && !this->copy_.contains(nh.key())) {
        this->copy_[nh.key()] = nullptr;
      }
      markAsUsed(); return this->value_.insert(hint, std::move(nh));
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
      if (!this->copy_.contains(k)) {
        this->copy_[k] = (this->value_.contains(k)) ? std::make_unique<T>(this->value_[k]) : nullptr;
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
      if (!this->copy_.contains(k)) {
        this->copy_[k] = (this->value_.contains(k)) ? std::make_unique<T>(this->value_[k]) : nullptr;
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
      if (!this->copy_.contains(k)) {
        this->copy_[k] = (this->value_.contains(k)) ? std::make_unique<T>(this->value_[k]) : nullptr;
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
      if (!this->copy_.contains(k)) {
        this->copy_[k] = (this->value_.contains(k)) ? std::make_unique<T>(this->value_[k]) : nullptr;
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
      if (!this->value_.contains(args.first) && !this->copy_.contains(args.first)) {
        this->copy_[args.first] = nullptr;
      }
      markAsUsed(); return this->value_.emplace(std::forward<Args>(args)...);
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
      if (!this->value_.contains(args.first) && !this->copy_.contains(args.first)) {
        this->copy_[args.first] = nullptr;
      }
      markAsUsed(); return this->value_.emplace(hint, std::forward<Args>(args)...);
    }

    /**
     * Try emplacing a value into the map, do nothing if key already exists.
     * @param key The key to emplace.
     * @param args The argument to build the value for emplace (not variadic! it's just one value).
     * @return A pair consisting of an iterator to the emplaced value and a
     *         boolean indicating whether the emplace was successful.
     */
    template <typename... Args> std::pair<
      typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool
    > try_emplace(const Key& key, Args&&... args) {
      if (!this->value_.contains(args.first) && !this->copy_.contains(args.first)) {
        this->copy_[args.first] = nullptr;
      }
      markAsUsed(); return this->value_.try_emplace(key, std::forward<Args>(args)...);
    }

    /**
     * Try emplacing a value into the map, using move/forward on the key.
     * @param key The key to emplace.
     * @param args The argument to build the value for emplace (not variadic! it's just one value).
     * @return A pair consisting of an iterator to the emplaced value and a
     *         boolean indicating whether the emplace was successful.
     */
    template <typename... Args> std::pair<
      typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool
    > try_emplace(Key&& key, Args&&... args) {
      if (!this->value_.contains(args.first) && !this->copy_.contains(args.first)) {
        this->copy_[args.first] = nullptr;
      }
      markAsUsed(); return this->value_.try_emplace(std::move(key), std::forward<Args>(args)...);
    }

    /**
     * Try emplacing a value into the map, using a hint.
     * @param hint The hint to use.
     * @param key The key to emplace.
     * @param args The argument to build the value for emplace (not variadic! it's just one value).
     * @return A pair consisting of an iterator to the emplaced value and a
     *         boolean indicating whether the emplace was successful.
     */
    template <typename... Args> std::pair<typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool>
    try_emplace(std::unordered_map<Key, T, SafeHash>::const_iterator hint, const Key& key, Args&&... args) {
      if (!this->value_.contains(args.first) && !this->copy_.contains(args.first)) {
        this->copy_[args.first] = nullptr;
      }
      markAsUsed(); return this->value_.try_emplace(hint, key, std::forward<Args>(args)...);
    }

    /**
     * Try emplacing a value into the map, using a hint and move/forward.
     * @param hint The hint to use.
     * @param key The key to emplace.
     * @param args The argument to build the value for emplace (not variadic! it's just one value).
     * @return A pair consisting of an iterator to the emplaced value and a
     *         boolean indicating whether the emplace was successful.
     */
    template <typename... Args> std::pair<typename std::unordered_map<Key, T, SafeHash>::const_iterator, bool>
    try_emplace(std::unordered_map<Key, T, SafeHash>::const_iterator hint, Key&& key, Args&&... args) {
      if (!this->value_.contains(args.first) && !this->copy_.contains(args.first)) {
        this->copy_[args.first] = nullptr;
      }
      markAsUsed(); return this->value_.try_emplace(hint, std::move(key), std::forward<Args>(args)...);
    }

    /**
     * Erase a value from the map.
     * @param pos The position of the value to erase.
     * @return An iterator to the next value.
     */
    typename std::unordered_map<Key, T, SafeHash>::const_iterator erase(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator pos
    ) {
      if (this->value_.contains((*pos).first) && !this->copy_.contains((*pos).first)) {
        this->copy_[(*pos).first] = std::make_unique<T>(this->value_[(*pos).first]);
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
      for (auto it = first; it < last; it++) {
        if (this->value_.contains((*it).first) && !this->copy_.contains((*it).first)) {
          this->copy_[(*it).first] = std::make_unique<T>(this->value_[(*it).first]);
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
      if (this->value_.contains(key) && !this->copy_.contains(key) {
        this->copy_[key] = std::make_unique<T>(this->value_[key]);
      }
      markAsUsed(); return this->value_.erase(key);
    }

    /**
     * Erase a value from the map, using a key and move/forward.
     * @param key The key of the value to erase.
     * @return The number of values erased.
     */
    template <class K> typename std::unordered_map<Key, T, SafeHash>::size_type erase(K&& key) {
      if (this->value_.contains(key) && !this->copy_.contains(key) {
        this->copy_[key] = std::make_unique<T>(this->value_[key]);
      }
      markAsUsed(); return this->value_.erase(std::forward<K>(key));
    }

    ///@{
    /** Swap the contents of two maps. Swaps only the CURRENT value. */
    inline void swap(std::unordered_map<Key, T, SafeHash>& other) {
      for (std::pair<Key, T> val : this->value_) {
        if (!this->copy_.contains(val.first)) this->copy_[val.first] = std::make_unique<T>(val.second);
      }
      markAsUsed(); this->value_.swap(other);
    }
    inline void swap(SafeUnorderedMap<Key, T, SafeHash>& other) {
      for (std::pair<Key, T> val : this->value_) {
        if (!this->copy_.contains(val.first)) this->copy_[val.first] = std::make_unique<T>(val.second);
      }
      markAsUsed(); other.markAsUsed(); this->value_.swap(other.value_);
    }
    ///@}

    ///@{
    /**
     * Get the value with the given key.
     * @param key The key to get the value from.
     * @return A reference to the value within the key.
     * @throws std::out_of_range if key does not exist in the map.
     */
    inline T& at(const Key& key) {
      if (!this->copy_.contains(key)) {
        if (this->value_.contains(key)) this->copy_[key] = std::make_unique<T>(this->value_[key]);
      }
      markAsUsed(); return this->value_.at(key);
    }
    inline const T& at(const Key& key) const { return this->value_.at(key); }
    ///@}

    ///@{
    /** Subscript/indexing operator. Creates the key if it doesn't exist. */
    T& operator[](const Key& key) {
      if (!this->copy_.contains(key)) {
        this->copy_[key] = (this->value_.contains(key)) ? std::make_unique<T>(this->value_[key]) : nullptr;
      }
      markAsUsed(); return this->value_[key];
    }
    T& operator[](Key&& key) {
      if (!this->copy_.contains(key)) {
        this->copy_[key] = (this->value_.contains(key)) ? std::make_unique<T>(this->value_[key]) : nullptr;
      }
      markAsUsed(); return this->value_[std::move(key)];
    }
    ///@}

    ///@{
    /** Assignment operator. Assigns only the CURRENT value. */
    SafeUnorderedMap& operator=(const std::unordered_map<Key, T, SafeHash>& map) {
      for (std::pair<Key, T> val : this->value_) {
        if (!this->copy_.contains(val.first)) this->copy_[val.first] = std::make_unique<T>(val.second);
      }
      markAsUsed(); this->value_ = map; return *this;
    }
    SafeUnorderedMap& operator=(const SafeUnorderedMap& other) {
      for (std::pair<Key, T> val : this->value_) {
        if (!this->copy_.contains(val.first)) this->copy_[val.first] = std::make_unique<T>(val.second);
      }
      markAsUsed(); this->value_ = other.get(); return *this;
    }
    ///@}

    /// Commit the value.
    void commit() override { this->copy_.clear(); this->registered_ = false; }

    /// Revert the value.
    void revert() const override {
      for (std::pair<Key, T> val : this->copy_) {
        if (val.second == nullptr) {
          this->value_.erase(val.first);
        } else {
          this->value_.insert_or_assign(val.first, *val.second);
        }
      }
      this->copy_.clear(); this->registered_ = false;
    }
};

#endif // SAFEUNORDEREDMAP_H
