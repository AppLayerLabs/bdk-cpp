#ifndef SAFEUNORDEREDMAP_H
#define SAFEUNORDEREDMAP_H

#include "../../utils/safehash.h"
#include "safebase.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>

/// TODO: somehow figure out a way to make loop (for (const auto& [key, value] :
/// map) { ... }) work with this class

/**
 * Safe wrapper for an unordered_map variable.
 * This class is used to safely store an unordered_map variable within a
 * contract.
 * @see SafeBase
 */
template <typename Key, typename T> class SafeUnorderedMap : public SafeBase {
private:
  std::unordered_map<Key, T, SafeHash> map; ///< unordered_map value.
  mutable std::unique_ptr<std::unordered_map<Key, T, SafeHash>>
      mapPtr; ///< Pointer to the unordered_map value.
  mutable std::unique_ptr<std::unordered_set<Key, SafeHash>>
      erasedKeys; ///< Pointer to the set of erased keys.

  /**
   *@brief Check if the mapPtr and erasedKeys are initialized.
   * If not, initialize them.
   */
  inline void check() const override {
    if (mapPtr == nullptr) {
      mapPtr = std::make_unique<std::unordered_map<Key, T, SafeHash>>();
    }
    if (erasedKeys == nullptr) {
      erasedKeys = std::make_unique<std::unordered_set<Key, SafeHash>>();
    }
  }

  /**
   *@brief Check if the key exists and only copy if it truly does.
   *@param key The key to check.
   */
  inline void checkKeyAndCopy(const Key &key) const {
    check();
    auto itP = mapPtr->find(key);
    if (itP == mapPtr->end()) {
      auto itM = map.find(key);
      if (itM != map.end()) {
        auto itD = erasedKeys->find(key);
        if (itD == erasedKeys->end()) {
          (*mapPtr)[key] = itM->second;
        }
      }
    }
  }

  /**
   *@brief Check if the key exists and creates a new one if doesn't.
   *@param key The key to check.
   */
  inline void checkKeyAndCreate(const Key &key) const {
    check();
    auto itP = mapPtr->find(key);
    if (itP == mapPtr->end()) {
      auto itM = map.find(key);
      if (itM == map.end()) {
        (*mapPtr)[key] = T();
      } else {
        auto itD = erasedKeys->find(key);
        if (itD == erasedKeys->end()) {
          (*mapPtr)[key] = itM->second;
        } else {
          (*mapPtr)[key] = T();
        }
      }
    }
  }

  /**
   *@brief Check if keys exists, throws if doesn't.
   *@param key The key to check.
   *@throws std::runtime_error if key doesn't exist.
   */
  inline void checkKeyAndThrow(const Key &key) const {
    check();
    auto itP = mapPtr->find(key);
    if (itP == mapPtr->end()) {
      auto itM = map.find(key);
      if (itM == map.end()) {
        throw std::runtime_error("Key not found");
      } else {
        auto itD = erasedKeys->find(key);
        if (itD == erasedKeys->end()) {
          (*mapPtr)[key] = itM->second;
        } else {
          throw std::runtime_error("Key not found");
        }
      }
    }
  }

public:
  /**
   *@brief Constructor for a SafeUnorderedMap.
   * Only variables built with this constructor will be registered within a
   * contract.
   *@param owner The contract that owns this variable.
   *@param map The unordered_map value.
   */
  SafeUnorderedMap(DynamicContract *owner,
                   const std::unordered_map<Key, T, SafeHash> &map = {})
      : SafeBase(owner), map(map) {}

  /**
   *@brief Normal constructor for a SafeUnorderedMap.
   *@param map The unordered_map value.
   */
  SafeUnorderedMap(const std::unordered_map<Key, T, SafeHash> &map = {})
      : SafeBase(nullptr),
        mapPtr(std::make_unique<std::unordered_map<Key, T, SafeHash>>(map)) {}

  /**
   *@brief Copy constructor for a SafeUnorderedMap.
   *@param other The SafeUnorderedMap to copy.
   */
  SafeUnorderedMap(const SafeUnorderedMap &other) : SafeBase(nullptr) {
    other.check();
    map = other.map;
    mapPtr =
        std::make_unique<std::unordered_map<Key, T, SafeHash>>(*other.mapPtr);
    erasedKeys =
        std::make_unique<std::unordered_set<Key, SafeHash>>(*other.erasedKeys);
  }

  /**
   *@brief Returns the original unordered_map value.
   *This function can only be used within a view/const function.
   *Iterating over it DOES NOT load temporary values.
   *@return The original unordered_map value.
   */
  inline std::unordered_map<Key, T>::const_iterator cbegin() const noexcept {
    return map.cbegin();
  }

  /**
   *@brief Returns the original unordered_map value, but with the end iterator.
   *This function can only be used within a view/const function.
   *Iterating over it DOES NOT load temporary values.
   *@return The original unordered_map value.
   */
  inline std::unordered_map<Key, T>::const_iterator cend() const noexcept {
    return map.cend();
  }

  /**
   *@brief Returns the begin of a temporary unordered_map value.
   *Can be used within a find() + end() combo.
   *Iterating over it DOES NOT load temporary values.
   *@return The unordered_map value begin.
   */
  inline std::unordered_map<Key, T>::iterator begin() const noexcept {
    check();
    return mapPtr->begin();
  }

  /**
   *@brief Returns the end of a temporary unordered_map value.
   *Can be used within a find() + end() combo.
   *Iterating over it DOES NOT load temporary values.
   *@return The unordered_map value end.
   */
  inline std::unordered_map<Key, T>::iterator end() const noexcept {
    check();
    return mapPtr->end();
  }

  /**
   *@brief Returns true if the unordered_map value is empty.
   *@param key The key to check.
   *@return True if the unordered_map value is empty, false otherwise.
   */
  inline bool empty() const noexcept {
    check();
    return (map.empty() || mapPtr->empty());
  }

  /// TODO: This can only be used within a view/const function
  /**
   *@brief Returns the size of the unordered_map.
   *@return The size of the unordered_map.
   */
  inline size_t size() const noexcept {
    check();
    return map.size();
  }

  /// TODO: Manage a way to implement: Loops, clear and iterators.
  /// std::pair<iterator,bool> insert( const value_type& value );

  /**
  *@brief Inserts a value into the unordered map and returns an iterator to
  the inserted element.
  *@param value The value to insert into the unordered map.
  *@return A pair consisting of an iterator to the inserted element and a
  boolean value indicating *whether the insertion was successful.
  */
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool>
  insert(
      const typename std::unordered_map<Key, T, SafeHash>::value_type &value) {
    check();
    markAsUsed();
    return mapPtr->insert(value);
  }

  /// std::pair<iterator,bool> insert( value_type&& value );

  /**
   *@brief Inserts a new value into the unordered_map, forwarding the value.
    map.
    *@param value The value to insert.
    *@return A pair with an iterator to the inserted value and a bool that
    indicates if the value was inserted or not.
    */
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool>
  insert(typename std::unordered_map<Key, T, SafeHash>::value_type &&value) {
    check();
    markAsUsed();
    return mapPtr->insert(std::move(value));
  }

  /// template< class P >
  /// std::pair<iterator,bool> insert( P&& value );

  /**
   *@brief Inserts a new value into the unordered_map, forwarding the value.
    *@param value The value to insert.
    *@return A pair with an iterator to the inserted value and a bool that
    indicates if the value was inserted or not.
    */
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool>
  insert(T &&value) {
    check();
    markAsUsed();
    return mapPtr->insert(std::forward<T>(value));
  }

  /// iterator insert( const_iterator hint, const value_type& value )

  /**
   *@brief Inserts a new value into the unordered_map, using a hint.
   *@param hint The hint to use (hint is the position before the insertion)
   *@param value The value to insert.
   *@return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator insert(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
      const typename std::unordered_map<Key, T, SafeHash>::value_type &value) {
    check();
    markAsUsed();
    return mapPtr->insert(hint, value);
  }

  /// iterator insert( const_iterator hint, value_type&& value );

  /**
   *@brief Inserts a new value into the unordered_map, using a hint and move
   *semantics.
   *@param hint The hint to use (hint is the position before the insertion)
   *@param value The value to insert.
   *@return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator
  insert(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
         typename std::unordered_map<Key, T, SafeHash>::value_type &&value) {
    check();
    markAsUsed();
    return mapPtr->insert(hint, std::move(value));
  }

  /// template< class P >
  /// iterator insert( const_iterator hint, P&& value );

  /**
   *@brief Inserts a new value into the unordered_map, using a hint and
   *forwarding the value.
   *@param hint The hint to use (hint is the position before the insertion)
   *@param value The value to insert.
   *@return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator
  insert(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
         T &&value) {
    check();
    markAsUsed();
    return mapPtr->insert(hint, std::forward<T>(value));
  }

  /// template< class InputIt >
  /// void insert( InputIt first, InputIt last );

  /**
   *@brief Inserts a range of values into the unordered_map.
   *@param first The first value of the range.
   *@param last The last value of the range.
   */
  template <class InputIt> void insert(InputIt first, InputIt last) {
    check();
    markAsUsed();
    mapPtr->insert(first, last);
  }

  /// void insert( std::initializer_list<value_type> ilist );

  /**
   *@brief Inserts a list of values into the unordered_map.
   *@param ilist The list of values to insert.
   */
  void insert(std::initializer_list<
              typename std::unordered_map<Key, T, SafeHash>::value_type>
                  ilist) {
    check();
    markAsUsed();
    mapPtr->insert(ilist);
  }

  /// insert_return_type insert( node_type&& nh );

  /**
   *@brief Inserts a new value into the unordered_map, using move semantics.
   *@param nh The value to insert.
   *@return A pair with an iterator to the inserted value and a bool that
   *indicates if the value was inserted or not.
   */
  typename std::unordered_map<Key, T, SafeHash>::insert_return_type
  insert(typename std::unordered_map<Key, T, SafeHash>::node_type &&nh) {
    check();
    markAsUsed();
    return mapPtr->insert(std::move(nh));
  }

  /// iterator insert( const_iterator hint, node_type&& nh );

  /**
   *@brief Inserts a new value into the unordered_map, using a hint and move
   *semantics.
   *@param hint The hint to use (hint is the position before the insertion)
   *@param nh The value to insert.
   *@return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator
  insert(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
         typename std::unordered_map<Key, T, SafeHash>::node_type &&nh) {
    check();
    markAsUsed();
    return mapPtr->insert(hint, std::move(nh));
  }

  /// std::pair<iterator, bool> insert_or_assign(const Key& k, const T& obj);

  /**
   *@brief Inserts a value into the unordered_map, or assigns it if the key
    *already exists.
    *@param k The key to insert.
    *@param obj The value to insert.
    *@return A pair with an iterator to the inserted value and a bool that
    indicates if the value was inserted or not.
    */
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool>
  insert_or_assign(const Key &k, const T &obj) {
    check();
    markAsUsed();
    return mapPtr->insert_or_assign(k, obj);
  }

  /// std::pair<iterator, bool> insert_or_assign(Key&& k, T&& obj);

  /**
   *@brief Inserts a value into the unordered_map, or assigns it if the key
    *already exists, using move semantics.
    *@param k The key to insert.
    *@param obj The value to insert.
    *@return A pair with an iterator to the inserted value and a bool that
    indicates if the value was inserted or not.
    */
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool>
  insert_or_assign(Key &&k, T &&obj) {
    check();
    markAsUsed();
    return mapPtr->insert_or_assign(std::move(k), std::move(obj));
  }

  /// iterator insert_or_assign(const_iterator hint, const Key& k, const T&
  /// obj);

  /**
   *@brief Inserts a value into the unordered_map, or assigns it if the key
   *already exists, using a hint.
   *@param hint The hint to use (hint is the position before the insertion)
   *@param k The key to insert.
   *@param obj The value to insert.
   *@return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator
  insert_or_assign(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
      const Key &k, const T &obj) {
    check();
    markAsUsed();
    return mapPtr->insert_or_assign(hint, k, obj);
  }

  /// iterator insert_or_assign(const_iterator hint, Key&& k, T&& obj);

  /**
   *@brief Inserts a value into the unordered_map, or assigns it if the key
   *already exists, using a hint and move semantics.
   *@param hint The hint to use (hint is the position before the insertion)
   *@param k The key to insert.
   *@param obj The value to insert.
   *@return An iterator to the inserted value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator
  insert_or_assign(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
      Key &&k, T &&obj) {
    check();
    markAsUsed();
    return mapPtr->insert_or_assign(hint, std::move(k), std::move(obj));
  }

  /// std::pair<iterator, bool> emplace(Args&&... args);

  /**
   *@brief Attempts to build and insert a value into the unordered_map.
    *@param args The arguments to build the value.
    *@return A pair with an iterator to the inserted value and a bool that
    indicates if the value was inserted or not.
    */
  template <typename... Args>
  const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool>
  emplace(Args &&...args) {
    check();
    markAsUsed();
    return mapPtr->emplace(std::forward<Args>(args)...);
  }

  /// iterator emplace_hint(const_iterator hint, Args&&... args);

  /**
   *@brief Attempts to build and insert a value into the unordered_map, using a
   *hint.
   *@param hint The hint to use (hint is the position before the insertion)
   *@param args The arguments to build the value.
   *@return An iterator to the inserted value.
   */
  template <typename... Args>
  const typename std::unordered_map<Key, T, SafeHash>::iterator emplace_hint(
      typename std::unordered_map<Key, T, SafeHash>::const_iterator hint,
      Args &&...args) {
    check();
    markAsUsed();
    return mapPtr->emplace_hint(hint, std::forward<Args>(args)...);
  }

  // iterator erase(iterator pos)

  /**
   *@brief Erases a value from the unordered_map.
   *@param pos The position of the value to erase.
   *@return An iterator to the next value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator
  erase(typename std::unordered_map<Key, T, SafeHash>::iterator pos) {
    check();
    markAsUsed();
    erasedKeys->insert(pos->first);
    return mapPtr->erase(pos);
  }

  // iterator erase(const_iterator pos)

  /**
   *@brief Erases a value from the unordered_map., using a const_iterator.
   *@param pos The position of the value to erase.
   *@return An iterator to the next value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator
  erase(typename std::unordered_map<Key, T, SafeHash>::const_iterator pos) {
    check();
    markAsUsed();
    erasedKeys->insert(pos->first);
    return mapPtr->erase(pos);
  }

  // iterator erase(const_iterator first, const_iterator last)

  /**
   *@brief Erases a range of values from the unordered_map.
   *@param first The first position to erase.
   *@param last The last position to erase.
   *@return An iterator to the next value.
   */
  const typename std::unordered_map<Key, T, SafeHash>::iterator
  erase(typename std::unordered_map<Key, T, SafeHash>::const_iterator first,
        typename std::unordered_map<Key, T, SafeHash>::const_iterator last) {
    check();
    markAsUsed();
    for (auto it = first; it != last; ++it) {
      erasedKeys->insert(it->first);
    }
    return mapPtr->erase(first, last);
  }

  // size_type erase(const Key& key)

  /**
   *@brief Erases a value from the unordered_map, using a key.
   *@param key The key of the value to erase.
   *@return The number of values erased.
   */
  typename std::unordered_map<Key, T, SafeHash>::size_type
  erase(const Key &key) {
    check();
    markAsUsed();
    erasedKeys->insert(key);
    return mapPtr->erase(key);
  }

  /// template<class K> size_type erase(K&& key)

  /**
   *@brief Erases a value from the unordered_map, using a key and move
   *semantics.
   *@param key The key of the value to erase.
   *@return The number of values erased.
   */
  template <class K>
  typename std::unordered_map<Key, T, SafeHash>::size_type erase(K &&key) {
    check();
    markAsUsed();
    erasedKeys->insert(std::forward<K>(key));
    return mapPtr->erase(std::forward<K>(key));
  }

  /// T& at(const Key& key);

  /**
   *@brief Returns a reference to the value with the given key.
   *@param key The key of the value to return.
   *@return A reference to the value with the given key.
   */
  T &at(const Key &key) {
    checkKeyAndThrow(key);
    markAsUsed();
    return (*mapPtr)[key];
  }

  /// const T& at(const Key& key) const;

  /**
   *@brief Returns a const reference to the value with the given key.
   *@param key The key of the value to return.
   *@return A const reference to the value with the given key.
   */
  const T &at(const Key &key) const {
    checkKeyAndThrow(key);
    return (*mapPtr)[key];
  }

  /// T& operator[](const Key& key);

  /**
   *@brief Returns a reference to the value with the given key.
   *@param key The key of the value to return.
   *@return A reference to the value with the given key.
   */
  T &operator[](const Key &key) {
    checkKeyAndCreate(key);
    markAsUsed();
    return (*mapPtr)[key];
  }

  /// T& operator[](Key&& key);

  /**
   *@brief Returns a reference to the value with the given key.
   *@param key The key of the value to return.
   *@return A reference to the value with the given key.
   */
  T &operator[](Key &&key) {
    checkKeyAndCreate(key);
    markAsUsed();
    return (*mapPtr)[key];
  }

  /// operator=.
  /// TODO: Can't really be used, because it would require a copy of the map.
  /// not reversible.

  /**
   *@brief Assigns a value to the unordered_map.
   *@param other The unordered_map to assign.
   *@return A reference to the unordered_map.
   */
  SafeUnorderedMap &operator=(const SafeUnorderedMap &other) {
    if (this != &other) {
      markAsUsed();
      other.check();
      map = other.map;
      mapPtr = std::make_unique(*other.mapPtr);
      erasedKeys = std::make_unique(*other.erasedKeys);
    }
    return *this;
  }

  /// size_type count(const Key& key) const;

  /**
   *@brief Returns the number of values with the given key.
   *@param key The key of the values to count.
   *@return The number of values with the given key.
   */
  size_t count(const Key &key) const {
    checkKeyAndCopy(key);
    return mapPtr->count(key);
  }

  /// iterator find(const Key& key);

  /**
   *@brief Returns an iterator to the value with the given key.
   *@param key The key of the value to return.
   *@return An iterator to the value with the given key.
   */
  typename std::unordered_map<Key, T, SafeHash>::iterator find(const Key &key) {
    checkKeyAndCopy(key);
    markAsUsed();
    return mapPtr->find(key);
  }

  /// const_iterator find(const Key& key) const;

  /**
   *@brief Returns a const iterator to the value with the given key.
   *@param key The key of the value to return.
   *@return A const iterator to the value with the given key.
   */
  const typename std::unordered_map<Key, T, SafeHash>::const_iterator
  find(const Key &key) const {
    checkKeyAndCopy(key);
    return mapPtr->find(key);
  }

  /// std::pair<iterator, iterator> equal_range(const Key& key);

  /**
   *@brief Returns true if the unordered_map contains the given key, false
   *otherwise.
   *@param key The key to check.
   *@return True if the unordered_map contains the given key, false otherwise.
   */
  bool contains(const Key &key) const {
    checkKeyAndCopy(key);
    return mapPtr->contains(key);
  }

  /**
Commit function used to commit the value of the SafeUnorderedMap to the value
pointed to.
*/
  void commit() override {
    check();
    map.merge(*mapPtr);
    for (const auto &key : (*erasedKeys)) {
      map.erase(key);
    }
    mapPtr = nullptr;
    registered = false;
  }

  /**
   Revert function used to revert the value of the SafeUnorderedMap (nullify
   it).
   */
  void revert() const override {
    mapPtr = nullptr;
    erasedKeys = nullptr;
    registered = false;
  }
};

#endif // SAFEUNORDEREDMAP_H
