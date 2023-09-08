#ifndef SAFEENUMERABLESET_H
#define SAFEENUMERABLESET_H

#include "safevector.h"
#include "safeunorderedmap.h"

/// Based on the OpenZeppelin EnumerableSet implementation.
/// Unsafe version - use SafeEnumerableSet if you need SafeVariable functionality.
template<typename T>
class EnumerableSet {
  private:
    /// Storage for the set values
    std::vector<T> values_;

    // Position of the value in the `values` array, plus 1 because index 0
    // means a value is not in the set.
    // We use uint64_t instead of uint256_t because the vector max size is uint64_t max
    std::unordered_map<T, uint64_t> indexes_;

    bool _add(const T& value) {
      if (!this->indexes_.contains(value)) {
        this->values_.push_back(value);
        this->indexes_[value] = this->values_.size();
        return true;
      }
      return false;
    }

    bool _remove(const T& value) {
      auto it = this->indexes_.find(value);
      if (it != this->indexes_.end()) {
        const uint64_t valueIndex = it->second;
        uint64_t toDeleteIndex = valueIndex - 1;
        uint64_t lastIndex = this->values_.size() - 1;
        if (toDeleteIndex != lastIndex) {
          T lastValue = this->values_[lastIndex];
          std::swap(this->values_[toDeleteIndex], this->values_[lastIndex]);
        }
        this->values_.pop_back();
        this->indexes_.erase(value);
        return true;
      }
      return false;
    }

    bool _contains(const T& value) const { return this->indexes_.contains(value); }

    bool _length() const { return this->values_.size(); }

    const T& _at(const uint64_t& index) const { return this->values_[index]; }

    const std::vector<T>& _values() const { return this->values_.get(); }

  public:
    EnumerableSet() = default;

    bool add(const T& value) { return this->_add(value); }

    bool remove(const T& value) { return this->_remove(value); }

    bool contains(const T& value) const { return this->_contains(value); }

    uint64_t length() const { return this->_length(); }

    const T& at(const uint64_t& index) const { return this->_at(index); }

    const std::vector<T>& values() const { return this->values_(); }
};

#endif  // ENUMERABLESET_H
