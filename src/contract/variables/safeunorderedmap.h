#ifndef SAFEUNORDEREDMAP_H
#define SAFEUNORDEREDMAP_H

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "safemaphash.h"
#include "../../utils/safehash.h"


/// TODO: somehow figure out a way to make loop (for (const auto& [key, value] : map) { ... }) work with this class
template<typename Key, typename T>
class SafeUnorderedMap {
  private:
    std::unordered_map<Key, T, SafeHash> map;
    mutable std::unique_ptr<std::unordered_map<Key, T, SafeHash>> mapPtr;
    mutable std::unique_ptr<std::unordered_set<Key, SafeHash>> erasedKeys;

    inline void check() const {
      if (mapPtr == nullptr) {
        mapPtr = std::make_unique<std::unordered_map<Key, T, SafeHash>>();
      }
      if (erasedKeys == nullptr) {
        erasedKeys = std::make_unique<std::unordered_set<Key, SafeHash>>();
      }
    }

    /// Check if the key exists and only copy if it truly does.
    /// Don't throw on error if doesn't.
    inline void checkKeyAndCopy(const Key& key) const {
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

    /// Check if the key exists and creates a new one if doesn't.
    inline void checkKeyAndCreate(const Key& key) const {
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

    /// Check if keys exists, throws if doesn't.
    inline void checkKeyAndThrow(const Key& key) const {
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
    SafeUnorderedMap(const std::unordered_map<Key, T, SafeHash>& map = {}) : mapPtr(std::make_unique<std::unordered_map<Key, T, SafeHash>>(map)) {}
    SafeUnorderedMap(const SafeUnorderedMap& other) {
      other.check();
      map = other.map;
      mapPtr = std::make_unique<std::unordered_map<Key, T, SafeHash>>(*other.mapPtr);
      erasedKeys = std::make_unique<std::unordered_set<Key, SafeHash>>(*other.erasedKeys);
    }

    /// this can only be used within a view/const function, returns original map, iteratin over it DOES NOT load temporary values.
    inline std::unordered_map<Key,T>::const_iterator cbegin() const noexcept { return map.cbegin(); }
    inline std::unordered_map<Key,T>::const_iterator cend() const noexcept { return map.cend(); }

    /// This returns begin and end of the tmp map, not the original map.
    /// Can be used within a find() + end() combo, iterating over it DOES NOT load temporary values.
    inline std::unordered_map<Key,T>::const_iterator begin() const noexcept { check(); return mapPtr->begin(); }
    inline std::unordered_map<Key,T>::const_iterator end() const noexcept { check(); return mapPtr->end(); }

    inline bool empty() const noexcept { check(); return (map.empty() || mapPtr->empty()); }
    /// TODO: This can only be used within a view/const function
    inline size_t size() const noexcept { check(); return map.size(); }


    /// TODO: Manage a way to implement: Loops, clear and iterators.
    /// std::pair<iterator,bool> insert( const value_type& value );
    const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool> insert(const typename std::unordered_map<Key, T, SafeHash>::value_type& value) {
      check();
      return mapPtr->insert(value);
    }

    /// std::pair<iterator,bool> insert( value_type&& value );
    const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool> insert(typename std::unordered_map<Key, T, SafeHash>::value_type&& value) {
      check();
      return mapPtr->insert(std::move(value));
    }

    /// template< class P >
    /// std::pair<iterator,bool> insert( P&& value );
    const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool> insert(T&& value) {
      check();
      return mapPtr->insert(std::forward<T>(value));
    }

    /// iterator insert( const_iterator hint, const value_type& value )
    const typename std::unordered_map<Key, T, SafeHash>::iterator insert(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, const typename std::unordered_map<Key, T, SafeHash>::value_type& value) {
      check();
      return mapPtr->insert(hint, value);
    }

    /// iterator insert( const_iterator hint, value_type&& value );
    const typename std::unordered_map<Key, T, SafeHash>::iterator insert(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, typename std::unordered_map<Key, T, SafeHash>::value_type&& value) {
      check();
      return mapPtr->insert(hint, std::move(value));
    }

    /// template< class P >
    /// iterator insert( const_iterator hint, P&& value );
    const typename std::unordered_map<Key, T, SafeHash>::iterator insert(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, T&& value) {
      check();
      return mapPtr->insert(hint, std::forward<T>(value));
    }

    /// template< class InputIt >
    /// void insert( InputIt first, InputIt last );
    template <class InputIt>
    void insert(InputIt first, InputIt last) {
      check();
      mapPtr->insert(first, last);
    }

    /// void insert( std::initializer_list<value_type> ilist );
    void insert(std::initializer_list<typename std::unordered_map<Key, T, SafeHash>::value_type> ilist) {
      check();
      mapPtr->insert(ilist);
    }

    /// insert_return_type insert( node_type&& nh );
    typename std::unordered_map<Key, T, SafeHash>::insert_return_type insert(typename std::unordered_map<Key, T, SafeHash>::node_type&& nh) {
      check();
      return mapPtr->insert(std::move(nh));
    }

    /// iterator insert( const_iterator hint, node_type&& nh );
    const typename std::unordered_map<Key, T, SafeHash>::iterator insert(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, typename std::unordered_map<Key, T, SafeHash>::node_type&& nh) {
      check();
      return mapPtr->insert(hint, std::move(nh));
    }

    /// std::pair<iterator, bool> insert_or_assign(const Key& k, const T& obj);
    const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool> insert_or_assign(const Key& k, const T& obj) {
      check();
      return mapPtr->insert_or_assign(k, obj);
    }

    /// std::pair<iterator, bool> insert_or_assign(Key&& k, T&& obj);
    const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool> insert_or_assign(Key&& k, T&& obj) {
      check();
      return mapPtr->insert_or_assign(std::move(k), std::move(obj));
    }

    /// iterator insert_or_assign(const_iterator hint, const Key& k, const T& obj);
    const typename std::unordered_map<Key, T, SafeHash>::iterator insert_or_assign(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, const Key& k, const T& obj) {
      check();
      return mapPtr->insert_or_assign(hint, k, obj);
    }

    /// iterator insert_or_assign(const_iterator hint, Key&& k, T&& obj);
    const typename std::unordered_map<Key, T, SafeHash>::iterator insert_or_assign(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, Key&& k, T&& obj) {
      check();
      return mapPtr->insert_or_assign(hint, std::move(k), std::move(obj));
    }

    /// std::pair<iterator, bool> emplace(Args&&... args);
    template<typename... Args>
    const std::pair<typename std::unordered_map<Key, T, SafeHash>::iterator, bool> emplace(Args&&... args) {
      check();
      return mapPtr->emplace(std::forward<Args>(args)...);
    }

    /// iterator emplace_hint(const_iterator hint, Args&&... args);
    template<typename... Args>
    const typename std::unordered_map<Key, T, SafeHash>::iterator emplace_hint(typename std::unordered_map<Key, T, SafeHash>::const_iterator hint, Args&&... args) {
      check();
      return mapPtr->emplace_hint(hint, std::forward<Args>(args)...);
    }

    /// iterator erase(iterator pos)
    const typename std::unordered_map<Key, T, SafeHash>::iterator erase(typename std::unordered_map<Key, T, SafeHash>::iterator pos) {
      check();
      erasedKeys->insert(pos->first);
      return mapPtr->erase(pos);
    }

    /// iterator erase(const_iterator pos)
    const typename std::unordered_map<Key, T, SafeHash>::iterator erase(typename std::unordered_map<Key, T, SafeHash>::const_iterator pos) {
      check();
      erasedKeys->insert(pos->first);
      return mapPtr->erase(pos);
    }

    /// iterator erase(const_iterator first, const_iterator last)
    const typename std::unordered_map<Key, T, SafeHash>::iterator erase(typename std::unordered_map<Key, T, SafeHash>::const_iterator first, typename std::unordered_map<Key, T, SafeHash>::const_iterator last) {
      check();
      for (auto it = first; it != last; ++it) {
        erasedKeys->insert(it->first);
      }
      return mapPtr->erase(first, last);
    }

    /// size_type erase(const Key& key)
    typename std::unordered_map<Key, T, SafeHash>::size_type erase(const Key& key) {
      check();
      erasedKeys->insert(key);
      return mapPtr->erase(key);
    }

    /// template<class K> size_type erase(K&& key)
    template<class K>
    typename std::unordered_map<Key, T, SafeHash>::size_type erase(K&& key) {
      check();
      erasedKeys->insert(std::forward<K>(key));
      return mapPtr->erase(std::forward<K>(key));
    }

    /// T& at(const Key& key);
    T& at(const Key& key) {
      checkKeyAndThrow(key);
      return (*mapPtr)[key];
    }

    /// const T& at(const Key& key) const;
    const T& at(const Key& key) const {
      checkKeyAndThrow(key);
      return (*mapPtr)[key];
    }

    /// T& operator[](const Key& key);
    T& operator[](const Key& key) {
      checkKeyAndCreate(key);
      return (*mapPtr)[key];
    }

    /// T& operator[](Key&& key);
    T& operator[](Key&& key) {
      checkKeyAndCreate(key);
      return (*mapPtr)[key];
    }

    /// operator=.
    /// TODO: Can't really be used, because it would require a copy of the map. not reversible.
    SafeUnorderedMap& operator=(const SafeUnorderedMap& other) {
      if (this != &other) {
        other.check();
        map = other.map;
        mapPtr = std::make_unique(*other.mapPtr);
        erasedKeys = std::make_unique(*other.erasedKeys);
      }
      return *this;
    }

    /// size_type count(const Key& key) const;
    size_t count(const Key& key) const {
      checkKeyAndCopy(key);
      return mapPtr->count(key);
    }

    /// iterator find(const Key& key);
    const typename std::unordered_map<Key, T, SafeHash>::iterator find(const Key& key) {
      checkKeyAndCopy(key);
      return mapPtr->find(key);
    }

    /// const_iterator find(const Key& key) const;
    typename std::unordered_map<Key, T, SafeHash>::const_iterator find(const Key& key) const {
      checkKeyAndCopy(key);
      return mapPtr->find(key);
    }

    bool contains(const Key& key) const {
      checkKeyAndCopy(key);
      return mapPtr->contains(key);
    }

    void commit() {
      check();
      for (const auto& [key, value] : (*mapPtr)) {
        map[key] = value;
      }
      for (const auto& key : (*erasedKeys)) {
        map.erase(key);
      }
      mapPtr = nullptr;
    }

    void revert() {
      mapPtr = nullptr;
      erasedKeys = nullptr;
    }
};

#endif // SAFEUNORDEREDMAP_H
