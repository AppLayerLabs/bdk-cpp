#ifndef SAFEENUMERABLEMAP_H
#define SAFEENUMERABLEMAP_H

#include "safeenumerableset.h"


template <typename Key, typename T>
class SafeEnumerableMap {
  private:
    SafeEnumerableSet<Key> keys_;
    SafeUnorderedMap<Key, T> values_;
  public:

    SafeEnumerableMap(DynamicContract* contract) : keys_(contract), values_(contract) {}

    bool set(const Key& key, const T& value) {
      this->values_[key] = value;
      return this->keys_.add(key);
    }

    bool remove(const Key& key) {
      this->values_.erase(key);
      return this->keys_.remove(key);
    }

    bool contains(const Key& key) const {
      return this->keys_.contains(key);
    }

    uint64_t length() const {
      return this->keys_.length();
    }

    std::tuple<Key, T> at(const uint64_t& index) const {
      Key key = this->keys_.at(index);
      return std::make_tuple<Key, T>(key, this->values_.at(key));
    }

    std::tuple<bool, T> tryGet(const Key& key, T& value) const {
      auto it = this->values_.find(key);
      if (it == this->values_.end()) {
        return std::make_tuple<bool, T>(this->contains(key), T());
      } else {
        return std::make_tuple<bool, T>(true, it->second);
      }
    }

    const T& get(const Key& key) const {
      auto it = this->values_.find(key);
      if (it == this->values_.end()) {
        if (this->contains(key)) {
          return T();
        } else {
          throw std::runtime_error("SafeEnumerableMap: Key not found");
        }
      } else {
        return it->second;
      }
    }

    const std::vector<T>& keys() const {
      return this->keys_.values();
    }
};








#endif // SAFEENUMERABLEMAP_H