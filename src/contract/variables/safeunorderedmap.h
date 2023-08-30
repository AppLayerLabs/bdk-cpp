/*
Copyright (c) [2023] [Sparq Network]

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

// TODO: somehow figure out a way to make loops work with this class
// (for (const auto& [key, value] : map) { ... })

/**
 * Safe wrapper for an unordered_map variable.
 * Used to safely store an unordered_map within a contract.
 * @see SafeBase
 */
template <typename Key, typename T> class SafeUnorderedMap : public SafeBase {
private:
  std::unordered_map<Key, T, SafeHash> map_; ///< Value.
  mutable std::unique_ptr<std::unordered_map<Key, T, SafeHash>> mapPtr_; ///< Pointer to the value.
  mutable std::unique_ptr<std::unordered_set<Key, SafeHash>> erasedKeys_; ///< Pointer to the set of erased keys.

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
      } else {
        auto itD = erasedKeys_->find(key);
        if (itD == erasedKeys_->end()) {
          (*mapPtr_)[key] = itM->second;
        } else {
          (*mapPtr_)[key] = T();
        }
      }
    }
  }

  /**
   * Check if a key exists, throw if it doesn't.
   * @param key The key to check.
   * @throw std::runtime_error if key doesn't exist.
   */
  inline void checkKeyAndThrow(const Key& key) const {
    check();
    auto itP = mapPtr_->find(key);
    if (itP == mapPtr_->end()) {
      auto itM = map_.find(key);
      if (itM == map_.end()) {
        throw std::runtime_error("Key not found");
      } else {
        auto itD = erasedKeys_->find(key);
        if (itD == erasedKeys_->end()) {
          (*mapPtr_)[key] = itM->second;
        } else {
          throw std::runtime_error("Key not found");
        }
      }
    }
  }

public:
  /**
   * Constructor.
   * @param owner The contract that owns the variable.
   * @param map The initial value. Defaults to an empty map.
   */
  SafeUnorderedMap(
    DynamicContract* owner, const std::unordered_map<Key, T, SafeHash>& map = {}
  ) : SafeBase(owner), map_(map) {}

  /**
   * Empty constructor.
   * @param map The initial value. Defaults to an empty map.
   */
  SafeUnorderedMap(const std::unordered_map<Key, T, SafeHash>& map = {})
    : SafeBase(nullptr), mapPtr_(std::make_unique<std::unordered_map<Key, T, SafeHash>>(map)) {}

  /// Copy constructor.
  SafeUnorderedMap(const SafeUnorderedMap& other) : SafeBase(nullptr) {
    other.check();
    map_ = other.map_;
    mapPtr_ = std::make_unique<std::unordered_map<Key, T, SafeHash>>(*other.mapPtr_);
    erasedKeys_ = std::make_unique<std::unordered_set<Key, SafeHash>>(*other.erasedKeys_);
  }

  /**
   * Get the number of values with the given key.
   * @param key The key of the values to count.
   * @return The number of values with the given key.
   */
  inline size_t count(const Key &key) const { checkKeyAndCopy(key); return mapPtr_->count(key); }

  /**
   * Find a given key.
   * @param key The key to find.
   * @return An iterator to the found key and its value.
   */
  typename std::unordered_map<Key, T, SafeHash>::iterator find(const Key& key) {
    checkKeyAndCopy(key); markAsUsed(); return mapPtr_->find(key);
  }

  /**
   * Find a given key.
   * @param key The key to find.
   * @return An const iterator to the found key and its value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::const_iterator find(const Key& key) const {
    checkKeyAndCopy(key); return mapPtr_->find(key);
  }

  /**
   * Check if the map contains a given key.
   * @param key The key to check.
   * @return `true` if the unordered_map contains the given key, `false` otherwise.
   */
  inline bool contains(const Key &key) const { checkKeyAndCopy(key); return mapPtr_->contains(key); }

  /**
   * Commit the value. Updates the values from the pointers, nullifies them
   * and unregisters the variable.
   */
  void commit() override {
    check();
    map_.merge(*mapPtr_);
    for (const auto &[key, value] : (*mapPtr_)) map_[key] = value;
    for (const auto& key : (*erasedKeys_)) map_.erase(key);
    mapPtr_ = nullptr;
    registered_ = false;
  }

  /// Revert the value. Nullifies the pointers and unregisters the variable.
  void revert() const override { mapPtr_ = nullptr; erasedKeys_ = nullptr; registered_ = false; }

  /**
   * Get an iterator to the start of the original map value.
   * This function can only be used within a view/const function.
   * Iterating over it DOES NOT load temporary values.
   * @return An iterator to the start of the original map.
   */
  inline std::unordered_map<Key, T>::const_iterator cbegin() const noexcept { return map_.cbegin(); }

  /**
   * Get an iterator to the end of the original map value.
   * This function can only be used within a view/const function.
   * Iterating over it DOES NOT load temporary values.
   * @return An iterator to the end of the original map.
   */
  inline std::unordered_map<Key, T>::const_iterator cend() const noexcept { return map_.cend(); }

  /**
   * Get an iterator to the start of the temporary map value.
   * Can be used within a find() + end() combo.
   * Iterating over it DOES NOT load temporary values.
   * @return An iterator to the start of the temporary map.
   */
  inline std::unordered_map<Key, T>::iterator begin() const noexcept { check(); return mapPtr_->begin(); }

  /**
   * Get an iterator to the end of the temporary map value.
   * Can be used within a find() + end() combo.
   * Iterating over it DOES NOT load temporary values.
   * @return An iterator to the end of the temporary map.
   */
  inline std::unordered_map<Key, T>::iterator end() const noexcept { check(); return mapPtr_->end(); }

  /**
   * Check if the map is empty (has no values).
   * Checks both original and temporary maps.
   * @return `true` if map is empty, `false` otherwise.
   */
  inline bool empty() const noexcept { check(); return (map_.empty() || mapPtr_->empty()); }

  /**
   * Get the size of the original map.
   * ATTENTION: Only use this with care, it only return the size of the original map..
   * @return The size of the original map.
   */
  inline size_t size() const noexcept { check(); return map_.size(); }

  // TODO: Find a way to implement loops, clear and iterators.

  /**
   * Insert a value into the map.
   * @param value The value to insert.
   * @return A pair consisting of an iterator to the inserted value and a
   *         boolean indicating whether the insertion was successful.
   */
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool> insert(
    const typename std::unordered_map<Key, T, SafeHash>::value_type& value
  ) {
    check(); markAsUsed(); return mapPtr_->insert(value);
  }

  /**
   * Insert a value into the map, using move.
   * @param value The value to insert.
   * @return A pair consisting of an iterator to the inserted value and a
   *         boolean indicating whether the insertion was successful.
   */
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool> insert(
    typename std::unordered_map<Key, T, SafeHash>::value_type&& value
  ) {
    check(); markAsUsed(); return mapPtr_->insert(std::move(value));
  }

  /**
   * Insert a value into the map, using forward.
   * @param value The value to insert.
   * @return A pair consisting of an iterator to the inserted value and a
   *         boolean indicating whether the insertion was successful.
   */
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool> insert(T&& value) {
    check(); markAsUsed(); return mapPtr_->insert(std::forward<T>(value));
  }

  /**
   * Insert a value into the map, using copy and a hint (the position before the insertion).
   * @param hint The hint to use.
   * @param value The value to insert.
   * @return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator insert(
    typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
    const typename std::unordered_map<Key, T, SafeHash>::value_type& value
  ) {
    check(); markAsUsed(); return mapPtr_->insert(hint, value);
  }

  /**
   * Insert a value into the map, using move and a hint (the position before the insertion).
   * @param hint The hint to use.
   * @param value The value to insert.
   * @return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator insert(
    typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
    typename std::unordered_map<Key, T, SafeHash>::value_type&& value
  ) {
    check(); markAsUsed(); return mapPtr_->insert(hint, std::move(value));
  }

  /**
   * Insert a value into the map, using forward and a hint (the position before the insertion).
   * @param hint The hint to use.
   * @param value The value to insert.
   * @return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator insert(
    typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, T&& value
  ) {
    check(); markAsUsed(); return mapPtr_->insert(hint, std::forward<T>(value));
  }

  /**
   * Insert a range of values into the map.
   * @param first An iterator to the first value of the range.
   * @param last An iterator to the last value of the range.
   */
  template <class InputIt> void insert(InputIt first, InputIt last) {
    check(); markAsUsed(); mapPtr_->insert(first, last);
  }

  /**
   * Insert a list of values into the map.
   * @param ilist The list of values to insert.
   */
  void insert(std::initializer_list<
    typename std::unordered_map<Key, T, SafeHash>::value_type
  > ilist) {
    check(); markAsUsed(); mapPtr_->insert(ilist);
  }

  /**
   * Insert a value into the map, using move with a node handle.
   * @param nh The value to insert.
   * @return A pair consisting of an iterator to the inserted value and a
   *         boolean indicating whether the insertion was successful.
   */
  typename std::unordered_map<Key, T, SafeHash>::insert_return_type
  insert(typename std::unordered_map<Key, T, SafeHash>::node_type&& nh) {
    check();
    markAsUsed();
    return mapPtr_->insert(std::move(nh));
  }

  /**
   * Insert a value into the map, using move with a node handle and a hint
   * (the position before the insertion).
   * @param nh The value to insert.
   * @param hint The hint to use.
   * @return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator insert(
    typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
    typename std::unordered_map<Key, T, SafeHash>::node_type&& nh
  ) {
    check(); markAsUsed(); return mapPtr_->insert(hint, std::move(nh));
  }

  /**
   * Insert a value into the map, or assign it if the key already exists.
   * @param k The key to insert.
   * @param obj The value to insert.
   * @return A pair consisting of an iterator to the inserted value and a
   *         boolean indicating whether the insertion was successful.
   */
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool> insert_or_assign(
    const Key& k, const T& obj
  ) {
    check(); markAsUsed(); return mapPtr_->insert_or_assign(k, obj);
  }

  /**
   * Insert a value into the map, or assign it if the key already exists, using move.
   * @param k The key to insert.
   * @param obj The value to insert.
   * @return A pair consisting of an iterator to the inserted value and a
   *         boolean indicating whether the insertion was successful.
   */
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool>
  insert_or_assign(Key&& k, T&& obj) {
    check();
    markAsUsed();
    return mapPtr_->insert_or_assign(std::move(k), std::move(obj));
  }

  /**
   * Insert a value into the map, or assign it if the key already exists,
   * using a hint (the position before the insertion).
   * @param hint The hint to use.
   * @param k The key to insert.
   * @param obj The value to insert.
   * @return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator insert_or_assign(
    typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
    const Key& k, const T& obj
  ) {
    check(); markAsUsed(); return mapPtr_->insert_or_assign(hint, k, obj);
  }

  /**
   * Insert a value into the map, or assign it if the key already exists,
   * using move and a hint (the position before the insertion).
   * @param hint The hint to use.
   * @param k The key to insert.
   * @param obj The value to insert.
   * @return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator insert_or_assign(
    typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
    Key&& k, T&& obj
  ) {
    check(); markAsUsed(); return mapPtr_->insert_or_assign(hint, std::move(k), std::move(obj));
  }

  /**
   * Emplace a value into the map.
   * @param args The arguments to build the value for insertion.
   * @return A pair consisting of an iterator to the inserted value and a
   *         boolean indicating whether the insertion was successful.
   */
  template <typename... Args> const std::pair<
    typename std::unordered_map<Key, T, SafeHash>::iterator, bool
  > emplace(Args&&... args) {
    check(); markAsUsed(); return mapPtr_->emplace(std::forward<Args>(args)...);
  }

  /**
   * Emplace a value into the map, using a hint (the position before the insertion).
   * @param hint The hint to use.
   * @param args The arguments to build the value for insertion.
   * @return An iterator to the inserted value.
   */
  template <typename... Args> const typename std::unordered_map<Key, T, SafeHash>::iterator
  emplace_hint(
    typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
    Args&& ...args
  ) {
    check(); markAsUsed(); return mapPtr_->emplace_hint(hint, std::forward<Args>(args)...);
  }

  /**
   * Erase a value from the map.
   * @param pos The position of the value to erase.
   * @return An iterator to the next value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator erase(
    typename std::unordered_map<Key, T, SafeHash>::iterator pos
  ) {
    check(); markAsUsed(); erasedKeys_->insert(pos->first); return mapPtr_->erase(pos);
  }

  /**
   * Erase a value from the map, using a const_iterator.
   * @param pos The position of the value to erase.
   * @return An iterator to the next value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator erase(
    typename std::unordered_map<Key, T, SafeHash>::const_iterator pos
  ) {
    check(); markAsUsed(); erasedKeys_->insert(pos->first); return mapPtr_->erase(pos);
  }

  /**
   * Erase a range of values from the map.
   * @param first The first position to erase.
   * @param last The last position to erase.
   * @return An iterator to the next value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator erase(
    typename std::unordered_map<Key, T, SafeHash>::const_iterator first,
    typename std::unordered_map<Key, T, SafeHash>::const_iterator last
  ) {
    check();
    markAsUsed();
    for (auto it = first; it != last; ++it) erasedKeys_->insert(it->first);
    return mapPtr_->erase(first, last);
  }

  /**
   * Erase a value from the map, using a key.
   * @param key The key of the value to erase.
   * @return The number of values erased.
   */
  typename std::unordered_map<Key, T, SafeHash>::size_type erase(const Key& key) {
    check(); markAsUsed(); erasedKeys_->insert(key); return mapPtr_->erase(key);
  }

  /**
   * Erase a value from the map, using a key and move/forward.
   * @param key The key of the value to erase.
   * @return The number of values erased.
   */
  template <class K> typename std::unordered_map<Key, T, SafeHash>::size_type erase(K&& key) {
    check(); markAsUsed(); erasedKeys_->insert(std::forward<K>(key));
    return mapPtr_->erase(std::forward<K>(key));
  }

  /**
   * Get the value with the given key.
   * @param key The key to get the value from.
   * @return A reference to the value within the key.
   */
  inline T& at(const Key& key) { checkKeyAndThrow(key); markAsUsed(); return (*mapPtr_)[key]; }

  /// Const overload of at().
  inline const T& at(const Key& key) const { checkKeyAndThrow(key); return (*mapPtr_)[key]; }

  /// Subscript/indexing operator. Creates the key if it doesn't exist.
  T& operator[](const Key& key) { checkKeyAndCreate(key); markAsUsed(); return (*mapPtr_)[key]; }

  /// Subscript/indexing operator. Creates the key if it doesn't exist.
  T& operator[](Key&& key) { checkKeyAndCreate(key); markAsUsed(); return (*mapPtr_)[key]; }

  // TODO: operator= can't really be used, because it would require a copy of the map, not reversible
  /// Assignment operator.
  SafeUnorderedMap& operator=(const SafeUnorderedMap& other) {
    if (this !=& other) {
      markAsUsed();
      other.check();
      map_ = other.map;
      mapPtr_ = std::make_unique(*other.mapPtr_);
      erasedKeys_ = std::make_unique(*other.erasedKeys_);
    }
    return *this;
  }
};

#endif // SAFEUNORDEREDMAP_H
