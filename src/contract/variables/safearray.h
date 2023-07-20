#ifndef SAFEARRAY_H
#define SAFEARRAY_H

#include <array>
#include <map>
#include "safebase.h"

/**
 * Safe wrapper for std::array
 * Same as std::vector
 * but with more contrains as a std::array is a fixed size container
 * SafeArray is ALWAYS initialized with default values, differently from std::array
 */

template <typename T, unsigned N>
class SafeArray : public SafeBase {
  private:
    std::array<T, N> array_; ///< The original array
    mutable std::unique_ptr<std::map<uint64_t, T>> tmp_; ///< The temporary array

    /// Check the tmp_ variables!
    inline void check() const {
      if (tmp_ == nullptr) {
        tmp_ = std::make_unique<std::map<uint64_t, T>>();
      }
    }

    /// Check a index and copy if necessary.
    inline void checkIndexAndCopy(const uint64_t& index) {
      this->check();
      if (index >= N) {
        throw std::out_of_range("Index out of range");
      }
      if (tmp_->contains(index)) {
        return;
      }
      tmp_->emplace(index, array_[index]);
    }

  public:

    /// Default Constructor
    SafeArray(const std::array<T, N>& a = {}) : SafeBase(nullptr) {
      check();
      uint64_t index = 0;
      for (const auto& value : a) {
        tmp_->emplace(index, value);
        ++index;
      }
    };

    /// Constructor for contracts
    SafeArray(DynamicContract* owner, const std::array<T, N>& a = {}) : SafeBase(owner) {
      check();
      uint64_t index = 0;
      for (const auto& value : a) {
        tmp_->emplace(index, value);
        ++index;
      }
    };

    /// Access specified element with bounds checking
    inline T& at(std::size_t pos) {
      checkIndexAndCopy(pos);
      markAsUsed();
      return tmp_->at(pos);
    }

    /// Access specified element with bounds checking (const version)
    const T& at(std::size_t pos) const {
      checkIndexAndCopy(pos);
      return tmp_->at(pos);
    }

    /// Access specified element
    inline T& operator[](std::size_t pos) {
      checkIndexAndCopy(pos);
      markAsUsed();
      return (*tmp_)[pos];
    }

    /// Access specified element (const version)
    inline const T& operator[](std::size_t pos) const {
      checkIndexAndCopy(pos);
      return (*tmp_)[pos];
    }

    /// Return the ORIGINAL array const begin()
    inline std::array<T, N>::const_iterator cbegin() const {
      return array_.cbegin();
    }

    /// Return the ORIGINAL array const end()
    inline std::array<T, N>::const_iterator cend() const {
      return array_.cend();
    }

    /// Return the ORIGINAL array const crbegin()
    inline std::array<T, N>::const_reverse_iterator crbegin() const {
      return array_.crbegin();
    }

    /// Return the ORIGINAL array const crend()
    inline std::array<T, N>::const_reverse_iterator crend() const {
      return array_.crend();
    }

    /// Empty
    inline bool empty() const {
      return (N == 0);
    }

    /// Size
    inline std::size_t size() const {
      return N;
    }

    /// Max size
    inline std::size_t max_size() const {
      return N;
    }

    /**
    * Fill the array with a value
    * @param value The value to fill the array with
    */
    inline void fill(const T& value) {
      for (uint64_t i = 0; i < N; ++i) {
        tmp_->insert_or_assign(i, value);
      }
    }

    /// Commit function.
    void commit() override {
      for (const auto& [index, value] : *tmp_) {
        array_[index] = value;
      }
    }

    /// Rollback function.
    void revert() const override {
      tmp_ = nullptr;
    }
};


#endif // SAFEARRAY_H