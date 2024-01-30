/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEARRAY_H
#define SAFEARRAY_H

#include <array>
#include <map>
#include "safebase.h"

/**
 * Safe wrapper for `std::array`.
 * Works the same as SafeVector, but with more constraints as a std::array is a fixed size container.
 * SafeArray is ALWAYS initialized with default values, differently from std::array.
 * @see SafeVector
 * @see SafeBase
 */
template <typename T, unsigned N> class SafeArray : public SafeBase {
  private:
    std::array<T, N> array_;                              ///< Original array.
    mutable std::unique_ptr<std::map<uint64_t, T>> tmp_;  ///< Temporary array.

    /// Check if the temporary array is initialized (and initialize it if not).
    inline void check() const {
      if (this->tmp_ == nullptr) this->tmp_ = std::make_unique<std::map<uint64_t, T>>();
    }

    /// Check a index and copy if necessary.
    /**
     * Check if a specific index exists in the temporary array, copying it from the original if not.
     * @param index The index to check.
     * @throw std::out_of_range if index is bigger than the maximum index of the array.
     */
    inline void checkIndexAndCopy_(const uint64_t& index) {
      this->check();
      if (index >= N) throw std::out_of_range("Index out of range");
      if (this->tmp_->contains(index)) return;
      this->tmp_->emplace(index, this->array_[index]);
    }

  public:
    /**
     * Default constructor.
     * @param a (optional) An array of T with fixed size of N to use during construction.
     *                      Defaults to an empty array.
     */
    SafeArray(const std::array<T, N>& a = {}) : SafeBase(nullptr) {
      check();
      uint64_t index = 0;
      for (const auto& value : a) {
        this->tmp_->emplace(index, value);
        index++;
      }
    };

    /**
     * Constructor with owner, for contracts.
     * @param owner The owner of the variable.
     * @param a (optional) An array of T with fixed size of N to use during construction.
     *                      Defaults to an empty array.
     */
    SafeArray(DynamicContract* owner, const std::array<T, N>& a = {}) : SafeBase(owner) {
      check();
      uint64_t index = 0;
      for (const auto& value : a) {
        this->tmp_->emplace(index, value);
        index++;
      }
    };

    /**
     * Access a specified element of the temporary array with bounds checking.
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    inline T& at(std::size_t pos) {
      checkIndexAndCopy_(pos);
      markAsUsed();
      return this->tmp_->at(pos);
    }

    /**
     * Const overload of at().
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    const T& at(std::size_t pos) const {
      checkIndexAndCopy_(pos);
      return this->tmp_->at(pos);
    }

    /**
     * Access a specified element of the temporary array without bounds checking.
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    inline T& operator[](std::size_t pos) {
      checkIndexAndCopy_(pos);
      markAsUsed();
      return (*this->tmp_)[pos];
    }

    /**
     * Const overload of operator[].
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    inline const T& operator[](std::size_t pos) const {
      checkIndexAndCopy_(pos);
      return (*this->tmp_)[pos];
    }

    /// Get an iterator to the beginning of the original array.
    inline std::array<T, N>::const_iterator cbegin() const { return this->array_.cbegin(); }

    /// Get an iterator to the end of the original array.
    inline std::array<T, N>::const_iterator cend() const { return this->array_.cend(); }

    /// Get a reverse iterator to the beginning of the original array.
    inline std::array<T, N>::const_reverse_iterator crbegin() const { return this->array_.crbegin(); }

    /// Get a reverse iterator to the end of the original array.
    inline std::array<T, N>::const_reverse_iterator crend() const { return this->array_.crend(); }

    /**
     * Check if the original array is empty (has no elements).
     * @return `true` if array is empty, `false` otherwise.
     */
    inline bool empty() const { return (N == 0); }

    /**
     * Get the current size of the original array.
     * @return The size of the array.
     */
    inline std::size_t size() const { return N; }

    /**
     * Get the maximum possible size of the original array.
     * @return The maximum size.
     */
    inline std::size_t max_size() const { return N; }

    /**
     * Fill the temporary array with a given value.
     * @param value The value to fill the array with.
     */
    inline void fill(const T& value) {
      for (uint64_t i = 0; i < N; i++) this->tmp_->insert_or_assign(i, value);
    }

    /// Commit the value. Updates the original array from the temporary one and clears it.
    void commit() override {
      for (const auto& [index, value] : *this->tmp_) this->array_[index] = value;
    }

    /// Revert the value. Nullifies the temporary array.
    void revert() const override { this->tmp_ = nullptr; }
};

#endif // SAFEARRAY_H
