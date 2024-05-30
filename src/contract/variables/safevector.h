/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEVECTOR_H
#define SAFEVECTOR_H

#include <array>
#include <iterator>
#include <stack>
#include <tuple>
#include <vector>

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
    /**
     * Enum for partial vector modifying operations, used by the undo structure.
     * Full operations are not included since doing any of them disables the
     * use of the undo stack from that point until commit/revert.
     * NOTE: RESIZE can be either partial or total - resize(0) = clear(), every other size (for now) is considered partial
     */
    enum VectorOp {
      AT, OPERATOR_BRACKETS, FRONT, BACK, INSERT, EMPLACE, ERASE, INSERT_BULK, ERASE_BULK,
      PUSH_BACK, EMPLACE_BACK, POP_BACK, RESIZE_MORE, RESIZE_LESS
    };

    /// Helper alias for the undo operation structure (operation made, in which index, optionally which quantity, and one or more old values).
    using UndoOp = std::tuple<VectorOp, std::size_t, std::size_t, std::vector<T>>;

    std::vector<T> value_; ///< Current ("original") value.
    std::unique_ptr<std::vector<T>> copy_; ///< Full copy of the current value.
    std::unique_ptr<std::stack<UndoOp, std::vector<UndoOp>>> undo_; ///< Undo stack of the current value.

    /// Undo all changes in the undo stack on top of the current value.
    void processUndoStack() {
      while (!this->undo_->empty()) {
        UndoOp op = this->undo_->top();
        switch (std::get<0>(op)) {
          case AT: this->value_.at(std::get<1>(op)) = std::get<3>(op)[0]; break;
          case OPERATOR_BRACKETS: this->value_[std::get<1>(op)] = std::get<3>(op)[0]; break;
          case FRONT: this->value_.at(0) = std::get<3>(op)[0]; break;
          case BACK: this->value_.at(this->value_.size() - 1) = std::get<3>(op)[0]; break;
          case INSERT:
          case EMPLACE: this->value_.erase(this->value_.begin() + std::get<1>(op)); break;
          case ERASE: this->value_.insert(this->value_.begin() + std::get<1>(op), std::get<3>(op)[0]); break;
          case INSERT_BULK:
            for (std::size_t i = 0; i < std::get<2>(op); i++) {
              this->value_.erase(this->value_.begin() + std::get<1>(op));
            }
            break;
          case ERASE_BULK:
            for (std::size_t i = 0; i < std::get<2>(op); i++) {
              this->value_.insert(this->value_.begin() + std::get<1>(op) + i, std::get<3>(op)[i]);
            }
            break;
          case PUSH_BACK:
          case EMPLACE_BACK: this->value_.pop_back(); break;
          case POP_BACK: this->value_.push_back(std::get<3>(op)[0]); break;
          // For resize(), treat index as quantity
          case RESIZE_MORE:
            for (std::size_t i = 0; i < std::get<1>(op); i++) this->value_.pop_back(); break;
          case RESIZE_LESS:
            for (std::size_t i = 0; i < std::get<1>(op); i++) this->value_.push_back(std::get<3>(op)[i]); break;
            break;
        }
        this->undo_->pop();
      }
    }

  public:
    /**
     * Constructor with owner.
     * @param owner The owner of the variable.
     * @param vec (optional) A vector of T to use during construction. Defaults to an empty vector.
     */
    explicit SafeVector(DynamicContract* owner, std::vector<T> vec = {})
      : SafeBase(owner), value_(vec), copy_(nullptr), undo_(nullptr) {}

    /**
     * Empty constructor.
     * @param vec (optional) A vector of T to use during construction. Defaults to an empty vector.
     */
    SafeVector(std::vector<T> vec = {}) : SafeBase(nullptr), value_(vec), copy_(nullptr), undo_(nullptr) {}

    /**
     * Constructor with repeating value.
     * @param count The number of copies to make.
     * @param value The value to copy.
     */
    SafeVector(std::size_t count, const T& value) :
      SafeBase(nullptr), value_(count, value), copy_(nullptr), undo_(nullptr) {}

    /**
     * Constructor with empty repeating value.
     * @param count The number of empty values to make.
     */
    explicit SafeVector(std::size_t count) : SafeBase(nullptr), value_(count), copy_(nullptr), undo_(nullptr) {}

    /**
     * Constructor with iterators.
     * @tparam InputIt Any iterator type.
     * @param first An iterator to the first value.
     * @param last An iterator to the last value.
     */
    template<class InputIt> SafeVector(InputIt first, InputIt last)
      : SafeBase(nullptr), value_(first, last), copy_(nullptr), undo_(nullptr) {}

    /**
     * Constructor with initializer list.
     * @param init The initializer list to use.
     */
    explicit SafeVector(std::initializer_list<T> init)
      : SafeBase(nullptr), value_(init), copy_(nullptr), undo_(nullptr) {}

    /// Copy constructor. Only copies the CURRENT value.
    SafeVector(const SafeVector& other) : SafeBase(nullptr), value_(other.value_), copy_(nullptr), undo_(nullptr) {}

    /// Get the inner vector (for const functions).
    inline const std::vector<T>& get() const { return this->value_; }

    /**
     * Replace the contents of the vector with copies of a value.
     * @param count The number of copies to make.
     * @param value The value to copy.
     */
    inline void assign(std::size_t count, const T& value) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::vector<T>>(this->value_);
      markAsUsed(); this->value_.assign(count, value);
    }

    /**
     * Replace the contents of the temporary vector with elements from the input range [first, last).
     * @tparam InputIt A type of iterator to the element.
     * @param first An iterator to the first element.
     * @param last An iterator to the last element.
     */
    template<class InputIt> inline void assign(InputIt first, InputIt last) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::vector<T>>(this->value_);
      markAsUsed(); this->value_.assign(first, last);
    }

    /**
     * Replace the contents with elements from an initializer list.
     * @param ilist The initializer list to use.
     */
    inline void assign(std::initializer_list<T> ilist) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::vector<T>>(this->value_);
      markAsUsed(); this->value_.assign(ilist);
    }

    ///@{
    /**
     * Access a specified element of the vector.
     * @param pos The position of the index to access.
     * @return The element at the given index.
     * @throws std::out_of_range if pos is bigger than the vector's size.
     */
    inline T& at(std::size_t pos) {
      T& ret = this->value_.at(pos);
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(VectorOp::AT, pos, 1, std::vector<T>{this->value_.at(pos)}));
      }
      markAsUsed(); return ret;
    }
    inline const T& at(std::size_t pos) const { return this->value_.at(pos); }
    ///@}

    ///@{
    /**
     * Access a specified element of the vector (without bounds checking).
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    inline T& operator[](std::size_t pos) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(VectorOp::OPERATOR_BRACKETS, pos, 1, std::vector<T>{this->value_[pos]}));
      }
      markAsUsed(); return this->value_[pos];
    }
    inline const T& operator[](std::size_t pos) const { return this->value_[pos]; }
    ///@}

    ///@{
    /** Access the first element of the vector. */
    inline T& front() {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(VectorOp::FRONT, 0, 1, std::vector<T>{this->value_.at(0)}));
      }
      markAsUsed(); return this->value_.front();
    }
    inline const T& front() const { return this->value_.front(); }
    ///@}

    ///@{
    /** Access the last element of the vector. */
    inline T& back() {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(VectorOp::BACK, this->value_.size() - 1, 1, std::vector<T>{this->value_.at(this->value_.size() - 1)}));
      }
      markAsUsed(); return this->value_.back();
    }
    inline const T& back() const { return this->value_.back(); }
    ///@}

    /// Get a pointer to the underlying array serving as element storage.
    inline const T* data() const { return this->value_.data(); }

    /// Get an iterator to the beginning of the vector.
    inline std::vector<T>::const_iterator cbegin() const { return this->value_.cbegin(); }

    /// Get an iterator to the end of the vector.
    inline std::vector<T>::const_iterator cend() const { return this->value_.cend(); }

    /// Get a reverse iterator to the beginning of the vector.
    inline std::vector<T>::const_reverse_iterator crbegin() const { return this->value_.crbegin(); }

    /// Get a reverse iterator to the end of the vector.
    inline std::vector<T>::const_reverse_iterator crend() const { return this->value_.crend(); }

    /// Check if the vector is empty.
    inline bool empty() const { return this->value_.empty(); }

    /// Get the vector's current size.
    inline std::size_t size() const { return this->value_.size(); }

    /// Get the vector's maximum size.
    inline std::size_t max_size() const { return this->value_.max_size(); }

    /**
     * Reserve space for a new cap of items in the vector, if the new cap is
     * greater than current capacity.
     * Does NOT change the vector's size or contents, therefore we don't
     * consider it for a copy or undo operation.
     * @param new_cap The new cap for the vector.
     */
    inline void reserve(std::size_t new_cap) { markAsUsed(); this->value_.reserve(new_cap); }

    /// Get the number of items the vector has currently allocated space for.
    inline std::size_t capacity() const { return this->value_.capacity(); }

    /**
     * Reduce unused capacity on the vector to fit the current size.
     * Does NOT change the vector's size or contents, therefore we don't
     * consider it for a copy or undo operation.
     */
    inline void shrink_to_fit() { markAsUsed(); this->value_.shrink_to_fit(); }

    /// Clear the vector.
    inline void clear() {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::vector<T>>(this->value_);
      markAsUsed(); this->value_.clear();
    }

    /**
     * Insert an element into the vector.
     * @param pos The position to insert.
     * @param value The element to insert.
     * @return An iterator to the element that was inserted.
     */
    std::vector<T>::const_iterator insert(std::vector<T>::const_iterator pos, const T& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.begin(), pos);
        this->undo_->emplace(std::make_tuple(VectorOp::INSERT, index, 1, std::vector<T>()));
      }
      markAsUsed(); return this->value_.insert(pos, value);
    }

    /**
     * Insert an element into the vector, using move.
     * @param pos The position to insert.
     * @param value The element to insert.
     * @return An iterator to the element that was inserted.
     */
    std::vector<T>::const_iterator insert(std::vector<T>::const_iterator pos, T&& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        this->undo_->emplace(std::make_tuple(VectorOp::INSERT, index, 1, std::vector<T>()));
      }
      markAsUsed(); return this->value_.insert(pos, std::move(value));
    }

    /**
     * Insert a repeated number of the same element into the vector.
     * @param pos The position to insert.
     * @param count The number of times to insert.
     * @param value The element to insert.
     * @return An iterator to the first element that was inserted.
     */
    std::vector<T>::const_iterator insert(std::vector<T>::const_iterator pos, std::size_t count, const T& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        this->undo_->emplace(std::make_tuple(VectorOp::INSERT_BULK, index, count, std::vector<T>()));
      }
      markAsUsed(); return this->value_.insert(pos, count, value);
    }

    /**
     * Insert a range of elements into the vector using iterators.
     * @param pos The position to insert.
     * @param first An iterator to the first value.
     * @param last An iterator to the last value.
     * @return An iterator to the first element that was inserted.
     */
    template<class InputIt> requires std::input_iterator<InputIt> std::vector<T>::const_iterator insert(
      std::vector<T>::const_iterator pos, InputIt first, InputIt last
    ) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        std::size_t diff = std::distance(first, last);
        this->undo_->emplace(std::make_tuple(VectorOp::INSERT_BULK, index, diff, std::vector<T>()));
      }
      markAsUsed(); return this->value_.insert(pos, first, last);
    }

    /**
     * Insert a list of elements into the vector.
     * @param pos The position to insert.
     * @param ilist The list of elements to insert.
     * @return An iterator to the first element that was inserted.
     */
    std::vector<T>::const_iterator insert(std::vector<T>::const_iterator pos, std::initializer_list<T> ilist) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        this->undo_->emplace(std::make_tuple(VectorOp::INSERT_BULK, index, ilist.size(), std::vector<T>()));
      }
      markAsUsed(); return this->value_.insert(pos, ilist);
    }

    /**
     * Emplace (construct in-place) an element into the vector.
     * @param pos The position to emplace.
     * @param args The element to emplace.
     * @return An iterator to the element that was emplaced.
     */
    template <class... Args> std::vector<T>::const_iterator emplace(std::vector<T>::const_iterator pos, Args&&... args) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(VectorOp::EMPLACE, std::distance(this->value_.cbegin(), pos), 1, std::vector<T>()));
      }
      markAsUsed(); return this->value_.emplace(pos, args...);
    }

    /**
     * Erase an element from the vector.
     * @param pos The index of the element to erase.
     * @return An iterator to the element after the removed one.
     */
    std::vector<T>::const_iterator erase(std::vector<T>::const_iterator pos) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        this->undo_->emplace(std::make_tuple(VectorOp::ERASE, index, 1, std::vector<T>{this->value_.at(index)}));
      }
      markAsUsed(); return this->value_.erase(pos);
    }

    /**
     * Erase a range of elements from the vector.
     * @param first An iterator to the first value.
     * @param last An iterator to the last value.
     * @return An iterator to the element after the last removed one.
     */
    std::vector<T>::const_iterator erase(
      std::vector<T>::const_iterator first, std::vector<T>::const_iterator last
    ) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), first);
        std::size_t diff = std::distance(first, last);
        std::vector<T> oldVals = std::vector<T>(first, last);
        this->undo_->emplace(std::make_tuple(VectorOp::ERASE_BULK, index, diff, oldVals));
      }
      markAsUsed(); return this->value_.erase(first, last);
    }

    /**
     * Append an element to the end of the vector.
     * @param value The value to append.
     */
    void push_back(const T& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(VectorOp::PUSH_BACK, 0, 0, std::vector<T>()));
      }
      markAsUsed(); this->value_.push_back(value);
    }

    /**
     * Append an element to the end of the vector, using move.
     * @param value The value to append.
     */
    void push_back(T&& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(VectorOp::PUSH_BACK, 0, 0, std::vector<T>()));
      }
      markAsUsed(); this->value_.push_back(std::move(value));
    }

    /**
     * Emplace an element at the end of the vector.
     * @param value The value to emplace.
     */
    T& emplace_back(T&& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(VectorOp::EMPLACE_BACK, 0, 0, std::vector<T>()));
      }
      markAsUsed(); return this->value_.emplace_back(value);
    }

    /// Erase the element at the end of the vector.
    void pop_back() {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(std::make_tuple(VectorOp::POP_BACK, 0, 0, std::vector<T>{this->value_.back()}));
      }
      markAsUsed(); this->value_.pop_back();
    }

    /**
     * Resize the vector to hold a given number of elements.
     * Default-constructed elements are appended.
     * @param count The number of items for the new size.
     */
    void resize(std::size_t count) {
      if (this->copy_ == nullptr) {
        if (count == 0) { // Treat as full operation if resize(0), otherwise treat as partial
          this->copy_ = std::make_unique<std::vector<T>>(this->value_);
        } else if (count != this->value_.size()) { // Only consider undo if size will actually change
          if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
          VectorOp vecOp; // RESIZE_MORE (will be bigger) or RESIZE_LESS (will be smaller)
          std::size_t diff = 0; // Size difference between old and new vector
          std::vector<T> vals = {}; // Old values from before the operation
          if (count > this->value_.size()) {
            vecOp = VectorOp::RESIZE_MORE;
            diff = count - this->value_.size();
          } else if (count < this->value_.size()) {
            vecOp = VectorOp::RESIZE_LESS;
            diff = this->value_.size() - count;
            vals = std::vector<T>(this->value_.end() - diff, this->value_.end());
          }
          this->undo_->emplace(std::make_tuple(vecOp, diff, 0, vals));
        }
      }
      markAsUsed(); this->value_.resize(count);
    }

    /**
     * Resize the vector to hold a given number of elements.
     * If new size is bigger, new elements are appended and initialized.
     * @param count The number of items for the new size.
     * @param value The value to append and initialize.
     */
    void resize(std::size_t count, const T& value) {
      if (this->copy_ == nullptr) {
        if (count == 0) { // Treat as full operation if resize(0), otherwise treat as partial
          this->copy_ = std::make_unique<std::vector<T>>(this->value_);
        } else if (count != this->value_.size()) { // Only consider undo if size will actually change
          if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
          VectorOp vecOp; // RESIZE_MORE (will be bigger) or RESIZE_LESS (will be smaller)
          std::size_t diff = 0; // Size difference between old and new vector
          std::vector<T> vals = {}; // Old values from before the operation
          if (count > this->value_.size()) {
            vecOp = VectorOp::RESIZE_MORE;
            diff = count - this->value_.size();
          } else if (count < this->value_.size()) {
            vecOp = VectorOp::RESIZE_LESS;
            diff = this->value_.size() - count;
            vals = std::vector<T>(this->value_.end() - diff, this->value_.end());
          }
          this->undo_->emplace(std::make_tuple(vecOp, diff, 0, vals));
        }
      }
      markAsUsed(); this->value_.resize(count, value);
    }

    ///@{
    /** Equality operator. Checks only the CURRENT value. */
    inline bool operator==(const std::vector<T>& other) const { return (this->value_ == other); }
    inline bool operator==(const SafeVector<T>& other) const { return (this->value_ == other.get()); }
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

#endif /// SAFEVECTOR_H
