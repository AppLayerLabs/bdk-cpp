/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEVECTOR_H
#define SAFEVECTOR_H

#include <vector>
#include <map>
#include "safebase.h"

/**
 * Safe wrapper for `std::vector`.
 * This class employs a `std::map` for temporary storage of changes to the vector,
 * ensuring efficient memory usage without necessitating a full vector copy or
 * initializing an entire vector of nullptrs for each access.
 * `std::map` is preferred over `std::unordered_map` due to its inherent ordering.
 * This allows safe and efficient access to indices within the current size of the vector.
 * Additionally, ordered iteration over newly accessed keys and prior keys is required.
 * For instance, with a vector of size 10, accessing indices 3, 5, 7, 10, 11, 12, 13 should
 * allow for sequential iteration over those exact indices when committing.
 * Trying to access elements out of bounds will throw an exception.
 * @tparam T Defines the type of the vector elements.
 * @see SafeBase
 */
template <typename T> class SafeVector : public SafeBase {
  private:
    std::vector<T> vector_; ///< The original vector.
    mutable std::unique_ptr<std::map<uint64_t, T>> tmp_; ///< The temporary map.
    mutable uint64_t maxIndex_ = 0; ///< The maximum index of the vector.
    mutable bool clear_ = false; ///< Whether the vector should be cleared.

    /// Check the tmp_ variables.
    inline void check() const {
      if (tmp_ == nullptr) {
        tmp_ = std::make_unique<std::map<uint64_t, T>>();
        maxIndex_ = vector_.size();
      }
    }

    /**
     * Check a specific index and copy it if necessary.
     * @param index The index to check.
     * @throw std::out_of_range if index goes beyond the vector's range.
     */
    inline void checkIndexAndCopy(const uint64_t& index) const {
      this->check();
      if (index >= maxIndex_) throw std::out_of_range("Index out of range");
      if (tmp_->contains(index)) return;
      tmp_->emplace(index, vector_[index]);
    }

  public:
    SafeVector() : SafeBase(nullptr) {};  ///< Default constructor.

    /**
     * Constructor with owner.
     * @param owner The owner of the variable.
     */
    explicit SafeVector(DynamicContract* owner) : SafeBase(owner) {};

    /**
     * Constructor with repeating value.
     * @param count The number of copies to make.
     * @param value The value to copy.
     */
    SafeVector(std::size_t count, const T& value) {
      check();
      for (std::size_t i = 0; i < count; i++) {
        tmp_->emplace(i, value);
        maxIndex_++;
      }
    }

    /**
     * Constructor with empty repeating value.
     * @param count The number of empty values to make.
     */
    explicit SafeVector(std::size_t count) {
      check();
      for (std::size_t i = 0; i < count; i++) {
        tmp_->emplace(i, T());
        maxIndex_++;
      }
    }

    /**
     * Constructor with iterators.
     * @tparam InputIt Any iterator type.
     * @param first An iterator to the first value.
     * @param last An iterator to the last value.
     */
    template<class InputIt> SafeVector(InputIt first, InputIt last) {
      check();
      uint64_t i = 0;
      for (auto it = first; it != last; it++, i++) {
        tmp_->emplace(i, *it);
        maxIndex_++;
      }
    }

    /**
     * Constructor with initializer list.
     * @param init The initializer list to use.
     */
    explicit SafeVector(std::initializer_list<T> init) {
      check(); for (const auto& val : init) { tmp_->emplace(maxIndex_, val); maxIndex_++; }
    }

    /// Copy constructor.
    SafeVector(const SafeVector& other) {
      check(); other.check(); *tmp_ = *(other.tmp_); maxIndex_ = other.maxIndex_;
    }

    /**
     * Replace the contents of the temporary vector with copies of a value.
     * @param count The number of copies to make.
     * @param value The value to copy.
     */
    inline void assign(std::size_t count, const T& value) {
      check();
      tmp_->clear();
      for (std::size_t i = 0; i < count; i++) tmp_->emplace(i, value);
      maxIndex_ = count;
      clear_ = true;
    }

    /**
     * Replace the contents of the temporary vector with elements from the input range [first, last).
     * @tparam InputIt A type of iterator to the element.
     * @param first An iterator to the first element.
     * @param last An iterator to the last element.
     */
    template<class InputIt> inline void assign(InputIt first, InputIt last) {
      check();
      tmp_->clear();
      uint64_t i = 0;
      for (auto it = first; it != last; it++, i++) tmp_->emplace(i, *it);
      maxIndex_ = i;
      clear_ = true;
    }

    /**
     * Replace the contents with elements from an initializer list.
     * @param ilist The initializer list to use.
     */
    inline void assign(std::initializer_list<T> ilist) {
      check();
      tmp_->clear();
      uint64_t i = 0;
      for (const auto& val : ilist) { tmp_->emplace(i, val); i++; }
      maxIndex_ = i;
      clear_ = true;
    }

    ///@{
    /**
     * Access a specified element with bounds checking.
     * @param pos The position of the element.
     */
    inline T& at(std::size_t pos) {
      checkIndexAndCopy(pos); markAsUsed(); return tmp_->at(pos);
    }
    const T& at(std::size_t pos) const {
      checkIndexAndCopy(pos); return tmp_->at(pos);
    }
    ///@}

    ///@{
    /**
     * Access a specified element without bounds checking.
     * @param pos The position of the element.
     */
    inline T& operator[](std::size_t pos) {
      checkIndexAndCopy(pos); markAsUsed(); return (*tmp_)[pos];
    }
    inline const T& operator[](std::size_t pos) const {
      checkIndexAndCopy(pos); return (*tmp_)[pos];
    }
    ///@}

    /// Return the ORIGINAL vector const begin()
    inline std::vector<T>::const_iterator cbegin() const { return vector_.cbegin(); }

    /// Return the ORIGINAL vector const end()
    inline std::vector<T>::const_iterator cend() const { return vector_.cend(); }

    /// Return the ORIGINAL vector const crbegin()
    inline std::vector<T>::const_reverse_iterator crbegin() const { return vector_.crbegin(); }

    /// Return the ORIGINAL vector const crend()
    inline std::vector<T>::const_reverse_iterator crend() const { return vector_.crend(); }

    /// Check if vector is empty.
    inline bool empty() const { return (maxIndex_ == 0); }

    /// Get the vector's size.
    inline std::size_t size() const { check(); return maxIndex_; }

    /// Get the vector's maximum size.
    inline std::size_t max_size() const { return std::numeric_limits<size_t>::max() - 1; }

    /// Clear the vector.
    inline void clear() { check(); markAsUsed(); tmp_->clear(); maxIndex_ = 0; clear_ = true; }

    /**
     * Insert an element into the vector.
     * This is not 1:1 - we use a uint64_t as the index of the inserted element,
     * because the temporary map can't return a std::vector<T>::iterator (because it is a map).
     * @param pos The position to insert.
     * @param value The value to insert.
     * @return The index of the element that was inserted.
     * @throw std::out_of_range if the insertion is done beyond the vector's range.
     */
    uint64_t insert(const uint64_t& pos, const T& value) {
      check();
      markAsUsed();
      if (pos == maxIndex_) {
        tmp_->insert_or_assign(pos, value);
        maxIndex_++;
        return pos;
      } else if (pos < maxIndex_) {
        /// Move all elements from pos to maxIndex_ one position to the right.
        /// So we can fit the new element at pos.
        for (uint64_t i = maxIndex_; i > pos; --i) {
          auto iter = tmp_->find(i - 1);
          if (iter != tmp_->end()) {
            tmp_->insert_or_assign(i, iter->second); // shift the element,
          } else {
            tmp_->insert_or_assign(i, vector_[i - 1]); // copy and shift the element from the original vector
          }
        }
        tmp_->insert_or_assign(pos, value);
        maxIndex_++;
        return pos;
      } else {
        throw std::out_of_range("pos out of range");
      }
    }

    /**
     * Erase an element from the vector.
     * @param pos The index of the element to erase.
     * @return The index of the first element following the erased element.
     */
    std::size_t erase(std::size_t pos) {
      checkIndexAndCopy(pos);
      markAsUsed();
      // Shift elements from the right of pos to fill the gap.
      for (std::size_t i = pos; i < maxIndex_ - 1; ++i) {
        auto iter = tmp_->find(i + 1);
        if (iter != tmp_->end()) {
          tmp_->insert_or_assign(i, iter->second); // Shift the element.
        } else {
          tmp_->insert_or_assign(i, vector_[i + 1]); // Copy and shift the element from the original vector.
        }
      }
      // Remove the last element.
      tmp_->erase(maxIndex_ - 1);
      maxIndex_--;
      return pos;
    }

    /**
     * Erase a range of elements from the vector.
     * @param first The index of the first element to remove.
     * @param last The index of the last element to remove.
     * @return The index of the first element following the erased element range.
     * @throw std::out_of_range if the erasing is done beyond the vector's range.
     */
    std::size_t erase(std::size_t first, std::size_t last) {
      check();
      markAsUsed();
      if (first > last || last > maxIndex_) throw std::out_of_range("Indices out of range");
      std::size_t numToRemove = last - first; // Compute the number of elements to be removed.
      // Shift elements from the right of last to fill the gap.
      for (std::size_t i = first; i < maxIndex_ - numToRemove; ++i) {
        auto iter = tmp_->find(i + numToRemove);
        if (iter != tmp_->end()) {
          tmp_->insert_or_assign(i, iter->second); // Shift the element
        } else {
          tmp_->insert_or_assign(i, vector_[i + numToRemove]); // Copy and shift the element from the original vector
        }
      }
      // Remove the last numToRemove elements.
      for (std::size_t i = 0; i < numToRemove; i++) tmp_->erase(maxIndex_ - 1 - i);
      maxIndex_ -= numToRemove;
      return first;
    }

    /**
     * Append an element to the end of the vector.
     * @param value The value to append.
     */
    void push_back(const T& value) {
      check(); markAsUsed(); tmp_->emplace(maxIndex_, value); maxIndex_++;
    }

    /**
     * Emplace an element at the end of the vector.
     * @param value The value to emplace.
     */
    void emplace_back(T&& value) {
      check(); markAsUsed(); tmp_->emplace(maxIndex_, std::move(value)); maxIndex_++;
    }

    /// Erase the element at the end of the vector.
    void pop_back() {
      check(); markAsUsed(); tmp_->erase(maxIndex_ - 1); maxIndex_--;
    }

    /**
     * Resize the vector to hold a given number of elements.
     * Default-constructed elements are appended.
     * @param count The number of items for the new size.
     */
    void resize(std::size_t count) {
      check();
      if (count < maxIndex_) {
        for (std::size_t i = count; i < maxIndex_; i++) tmp_->erase(i);
      } else if (count > maxIndex_) {
        for (std::size_t i = maxIndex_; i < count; i++) tmp_->emplace(i, T());
      }
      maxIndex_ = count;
      markAsUsed();
    }

    /**
     * Resize the vector to hold a given number of elements.
     * If new size is bigger, new elements are appended and initialized.
     * @param count The number of items for the new size.
     * @param value The value to append and initialize.
     */
    void resize(std::size_t count, const T& value) {
      check();
      if (count < maxIndex_) {
        for (std::size_t i = count; i < maxIndex_; i++) tmp_->erase(i);
      } else if (count > maxIndex_) {
        for (std::size_t i = maxIndex_; i < count; i++) tmp_->emplace(i, value);
      }
      maxIndex_ = count;
      markAsUsed();
    }

    /// Commit function.
    void commit() override {
      check();
      if (clear_) { vector_.clear(); clear_ = false; }
      // Erase difference in size.
      if (vector_.size() > maxIndex_) vector_.erase(vector_.begin() + maxIndex_, vector_.end());
      for (auto& it : *tmp_) {
        if (it.first < vector_.size()) {
          vector_[it.first] = it.second;
        } else {
          vector_.emplace_back(it.second);
        }
      }
      maxIndex_ = vector_.size();
    }

    /// Rollback function.
    void revert() const override { tmp_ = nullptr; clear_ = false; maxIndex_ = vector_.size(); }

    /// Get the inner vector (for const functions).
    inline const std::vector<T>& get() const { return this->vector_; }
};

#endif /// SAFEVECTOR_H
