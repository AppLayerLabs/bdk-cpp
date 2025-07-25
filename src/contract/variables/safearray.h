/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEARRAY_H
#define SAFEARRAY_H

#include <array>
#include <stack>
#include <tuple>
#include <vector>
#include <iostream>

#include "safebase.h"

/**
 * Safe wrapper for `std::array`.
 * Works the same as SafeVector, but with more constraints as a std::array is a fixed size container.
 * SafeArray is ALWAYS initialized with default values, differently from std::array.
 * @see SafeVector, SafeBase
 */
template <typename T, unsigned N> class SafeArray : public SafeBase {
  private:
    /**
     * Enum for partial array modifying operations, used by the undo structure.
     * Full operations are not included since doing any of them disables the
     * use of the undo stack from that point until commit/revert.
     */
    enum ArrayOp { AT, OPERATOR_BRACKETS, FRONT, BACK }; // Technically everything can be AT here, but discernment is important

    /// Helper alias for the undo operation structure (operation made, in which index, and the old value).
    using UndoOp = std::tuple<ArrayOp, std::size_t, T>;

    std::array<T,N> value_;  ///< Current ("original") value.
    std::unique_ptr<std::array<T,N>> copy_; ///< Full copy of the current value.
    std::unique_ptr<std::stack<UndoOp, std::vector<UndoOp>>> undo_; ///< Undo stack of the current value.

    /// Undo all changes in the undo stack on top of the current value.
    void processUndoStack() {
      while (!this->undo_->empty()) {
        const UndoOp& op = this->undo_->top();
        const auto& [operation, index, value] = op;
        switch (operation) {
          case AT: this->value_.at(index) = value; break;
          case OPERATOR_BRACKETS: this->value_[index] = value; break;
          case FRONT: this->value_.at(0) = value; break;
          case BACK: this->value_.at(N-1) = value; break;
          // at(0)/(N-1) are hardcoded on purpose - std::get<1>(op) is not really
          // needed for FRONT and BACK, but it could be used as well
        }
        this->undo_->pop();
      }
    }

  public:
    /**
     * Default constructor.
     * @param a (optional) An array of T with fixed size of N to use during construction. Defaults to an empty array.
     */
    explicit SafeArray(const std::array<T,N>& a = {}) : SafeBase(nullptr), value_(a), copy_(nullptr), undo_(nullptr) {}

    /**
     * Constructor with owner, for contracts.
     * @param owner The owner of the variable.
     * @param a (optional) An array of T with fixed size of N to use during construction. Defaults to an empty array.
     */
    explicit SafeArray(DynamicContract* owner, const std::array<T, N>& a = {})
      : SafeBase(owner), value_(a), copy_(nullptr), undo_(nullptr) {}

    ///@{
    /**
     * Access a specified element of the array.
     * @param pos The position of the index to access.
     * @return The element at the given index.
     * @throws std::out_of_range if pos is bigger than the array's size.
     */
    inline T& at(std::size_t pos) {
      T& ret = this->value_.at(pos);
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(ArrayOp::AT, pos, this->value_.at(pos)));
      }
      markAsUsed(); return ret;
    }
    inline const T& at(std::size_t pos) const { return this->value_.at(pos); }
    ///@}

    ///@{
    /**
     * Access a specified element of the array (without bounds checking).
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    inline T& operator[](std::size_t pos) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(ArrayOp::OPERATOR_BRACKETS, pos, this->value_[pos]));
      }
      markAsUsed(); return this->value_[pos];
    }
    inline const T& operator[](std::size_t pos) const { return this->value_[pos]; }
    ///@}

    ///@{
    /** Access the first element of the array. */
    inline T& front() {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(ArrayOp::FRONT, 0, this->value_.at(0)));
      }
      markAsUsed(); return this->value_.front();
    }
    inline const T& front() const { return this->value_.front(); }
    ///@}

    ///@{
    /** Access the last element of the array. */
    inline T& back() {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(ArrayOp::BACK, N-1, this->value_.at(N-1)));
      }
      markAsUsed(); return this->value_.back();
    }
    inline const T& back() const { return this->value_.back(); }
    ///@}

    /// Get a pointer to the underlying array serving as element storage.
    inline const T* data() const { return this->value_.data(); }

    /// Get an iterator to the beginning of the array.
    inline typename std::array<T, N>::const_iterator cbegin() const { return this->value_.cbegin(); }

    /// Get an iterator to the end of the array.
    inline typename std::array<T, N>::const_iterator cend() const { return this->value_.cend(); }

    /// Get a reverse iterator to the beginning of the array.
    inline typename std::array<T, N>::const_reverse_iterator crbegin() const { return this->value_.crbegin(); }

    /// Get a reverse iterator to the end of the array.
    inline typename std::array<T, N>::const_reverse_iterator crend() const { return this->value_.crend(); }

    /**
     * Check if the array is empty (has no elements).
     * @return `true` if array is empty, `false` otherwise.
     */
    constexpr bool empty() const noexcept { return this->value_.empty(); }

    /// Get the current size of the array.
    constexpr std::size_t size() const noexcept { return this->value_.size(); }

    /// Get the maximum possible size of the array.
    constexpr std::size_t max_size() const noexcept { return this->value_.max_size(); }

    /**
     * Fill the array with a given value.
     * @param value The value to fill the array with.
     */
    inline void fill(const T& value) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::array<T,N>>(this->value_);
      markAsUsed(); this->value_.fill(value);
    }

    ///@{
    /** Equality operator. Checks only the CURRENT value. */
    inline bool operator==(const std::array<T,N>& other) const { return (this->value_ == other); }
    inline bool operator==(const SafeArray<T,N>& other) const { return (this->value_ == other.value_); }
    ///@}

    /// Commit the value.
    void commit() override { this->copy_ = nullptr; this->undo_ = nullptr; this->registered_ = false; }

    /// Revert the value.
    void revert() override {
      if (this->copy_ != nullptr) this->value_ = *this->copy_;
      if (this->undo_ != nullptr && !this->undo_->empty()) this->processUndoStack();
      this->copy_ = nullptr; this->undo_ = nullptr; this->registered_ = false;
    }
};

#endif // SAFEARRAY_H
