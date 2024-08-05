/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEBYTES_H
#define SAFEBYTES_H

#include <iterator>
#include <stack>
#include <tuple>
#include <vector>

#include "safebase.h"

/**
 * Safe wrapper for a raw bytes container. Derived from SafeVector.
 * @see SafeVector
 */
class SafeBytes : public SafeBase {
  private:
    /**
     * Enum for partial bytes modifying operations, used by the undo structure.
     * Full operations are not included since doing any of them disables the
     * use of the undo stack from that point until commit/revert.
     * NOTE: RESIZE can be either partial or total - resize(0) = clear(), every other size (for now) is considered partial
     */
    enum BytesOp {
      AT, OPERATOR_BRACKETS, FRONT, BACK, INSERT, EMPLACE, ERASE, INSERT_BULK, ERASE_BULK,
      PUSH_BACK, EMPLACE_BACK, POP_BACK, RESIZE_MORE, RESIZE_LESS
    };

    /// Helper alias for the undo operation structure (operation made, in which index, optionally which quantity, and one or more old values).
    using UndoOp = std::tuple<BytesOp, std::size_t, std::size_t, std::vector<uint8_t>>;

    std::vector<uint8_t> value_; ///< Current ("original") value.
    std::unique_ptr<std::vector<uint8_t>> copy_; ///< Full copy of the current value.
    std::unique_ptr<std::stack<UndoOp, std::vector<UndoOp>>> undo_; ///< Undo stack of the current value.

    /// Undo all changes in the undo stack on top of the current value.
    void processUndoStack() {
      while (!this->undo_->empty()) {
        const auto& op = this->undo_->top();
        const auto& [opType, index, quantity, oldVals] = op;
        switch (opType) {
          case AT: this->value_.at(index) = oldVals[0]; break;
          case OPERATOR_BRACKETS: this->value_[index] = oldVals[0]; break;
          case FRONT: this->value_.at(0) = oldVals[0]; break;
          case BACK: this->value_.at(this->value_.size() - 1) = oldVals[0]; break;
          case INSERT:
          case EMPLACE: this->value_.erase(this->value_.begin() + index); break;
          case ERASE: this->value_.insert(this->value_.begin() + index, oldVals[0]); break;
          case INSERT_BULK:
            for (std::size_t i = 0; i < quantity; i++) {
              this->value_.erase(this->value_.begin() + index);
            }
            break;
          case ERASE_BULK:
            for (std::size_t i = 0; i < quantity; i++) {
              this->value_.insert(this->value_.begin() + index + i, oldVals[i]);
            }
            break;
          case PUSH_BACK:
          case EMPLACE_BACK: this->value_.pop_back(); break;
          case POP_BACK: this->value_.push_back(oldVals[0]); break;
          // For resize(), treat index as quantity
          case RESIZE_MORE:
            for (std::size_t i = 0; i < index; i++) this->value_.pop_back(); break;
          case RESIZE_LESS:
            for (std::size_t i = 0; i < index; i++) this->value_.push_back(oldVals[i]); break;
            break;
        }
        this->undo_->pop();
      }
    }

  public:
    /**
     * Constructor with owner.
     * @param owner The owner of the variable.
     * @param bytes (optional) A vector of bytes to use during construction. Defaults to an empty vector.
     */
    explicit SafeBytes(DynamicContract* owner, const std::vector<uint8_t>& bytes = {})
      : SafeBase(owner), value_(bytes), copy_(nullptr), undo_(nullptr) {}

    /**
     * Empty constructor.
     * @param bytes (optional) A vector of bytes to use during construction. Defaults to an empty vector.
     */
    explicit SafeBytes(const std::vector<uint8_t>& bytes = {}) : SafeBase(nullptr), value_(bytes), copy_(nullptr), undo_(nullptr) {}

    /**
     * Constructor with repeating value.
     * @param count The number of copies to make.
     * @param value The value to copy.
     */
    SafeBytes(std::size_t count, const uint8_t& value) :
      SafeBase(nullptr), value_(count, value), copy_(nullptr), undo_(nullptr) {}

    /**
     * Constructor with empty repeating value.
     * @param count The number of empty values to make.
     */
    explicit SafeBytes(std::size_t count) : SafeBase(nullptr), value_(count), copy_(nullptr), undo_(nullptr) {}

    /**
     * Constructor with iterators.
     * @tparam InputIt Any iterator type.
     * @param first An iterator to the first value.
     * @param last An iterator to the last value.
     */
    template<class InputIt> SafeBytes(InputIt first, InputIt last)
      : SafeBase(nullptr), value_(first, last), copy_(nullptr), undo_(nullptr) {}

    /**
     * Constructor with initializer list.
     * @param init The initializer list to use.
     */
    SafeBytes(std::initializer_list<uint8_t> init)
      : SafeBase(nullptr), value_(init), copy_(nullptr), undo_(nullptr) {}

    /// Copy constructor. Only copies the CURRENT value.
    SafeBytes(const SafeBytes& other) : SafeBase(nullptr), value_(other.value_), copy_(nullptr), undo_(nullptr) {}

    /// Get the inner vector (for const functions).
    inline const std::vector<uint8_t>& get() const { return this->value_; }

    /**
     * Replace the contents of the bytes with copies of a value.
     * @param count The number of copies to make.
     * @param value The value to copy.
     */
    inline void assign(std::size_t count, const uint8_t& value) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::vector<uint8_t>>(this->value_);
      markAsUsed(); this->value_.assign(count, value);
    }

    /**
     * Replace the contents of the bytes with elements from the input range [first, last).
     * @tparam InputIt A type of iterator to the element.
     * @param first An iterator to the first element.
     * @param last An iterator to the last element.
     */
    template<class InputIt> inline void assign(InputIt first, InputIt last) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::vector<uint8_t>>(this->value_);
      markAsUsed(); this->value_.assign(first, last);
    }

    /**
     * Replace the contents with elements from an initializer list.
     * @param ilist The initializer list to use.
     */
    inline void assign(const std::initializer_list<uint8_t>& ilist) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::vector<uint8_t>>(this->value_);
      markAsUsed(); this->value_.assign(ilist);
    }

    ///@{
    /**
     * Access a specified element of the bytes.
     * @param pos The position of the index to access.
     * @return The element at the given index.
     * @throws std::out_of_range if pos is bigger than the bytes' size.
     */
    inline uint8_t& at(std::size_t pos) {
      uint8_t& ret = this->value_.at(pos);
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(BytesOp::AT, pos, 1, std::vector<uint8_t>{this->value_.at(pos)});
      }
      markAsUsed(); return ret;
    }
    inline const uint8_t& at(std::size_t pos) const { return this->value_.at(pos); }
    ///@}

    ///@{
    /**
     * Access a specified element of the bytes (without bounds checking).
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    inline uint8_t& operator[](std::size_t pos) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(BytesOp::OPERATOR_BRACKETS, pos, 1, std::vector<uint8_t>{this->value_[pos]});
      }
      markAsUsed(); return this->value_[pos];
    }
    inline const uint8_t& operator[](std::size_t pos) const { return this->value_[pos]; }
    ///@}

    ///@{
    /** Access the first element of the vector. */
    inline uint8_t& front() {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(BytesOp::FRONT, 0, 1, std::vector<uint8_t>{this->value_.at(0)});
      }
      markAsUsed(); return this->value_.front();
    }
    inline const uint8_t& front() const { return this->value_.front(); }
    ///@}

    ///@{
    /** Access the last element of the vector. */
    inline uint8_t& back() {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(BytesOp::BACK, this->value_.size() - 1, 1, std::vector<uint8_t>{this->value_.at(this->value_.size() - 1)});
      }
      markAsUsed(); return this->value_.back();
    }
    inline const uint8_t& back() const { return this->value_.back(); }
    ///@}

    /// Get a pointer to the underlying array serving as element storage.
    inline const uint8_t* data() const { return this->value_.data(); }

    /// Get an iterator to the beginning of the bytes.
    inline typename std::vector<uint8_t>::const_iterator cbegin() const { return this->value_.cbegin(); }

    /// Get an iterator to the end of the bytes.
    inline typename std::vector<uint8_t>::const_iterator cend() const { return this->value_.cend(); }

    /// Get a reverse iterator to the beginning of the bytes.
    inline typename std::vector<uint8_t>::const_reverse_iterator crbegin() const { return this->value_.crbegin(); }

    /// Get a reverse iterator to the end of the bytes.
    inline typename std::vector<uint8_t>::const_reverse_iterator crend() const { return this->value_.crend(); }

    /// Check if the bytes is empty.
    inline bool empty() const { return this->value_.empty(); }

    /// Get the bytes' current size.
    inline std::size_t size() const { return this->value_.size(); }

    /// Get the bytes' maximum size.
    inline std::size_t max_size() const { return this->value_.max_size(); }

    /**
     * Reserve space for a new cap of items in the bytes, if the new cap is
     * greater than current capacity.
     * Does NOT change the bytes' size or contents, therefore we don't
     * consider it for a copy or undo operation.
     * @param new_cap The new cap for the bytes.
     */
    inline void reserve(std::size_t new_cap) { markAsUsed(); this->value_.reserve(new_cap); }

    /// Get the number of items the bytes has currently allocated space for.
    inline std::size_t capacity() const { return this->value_.capacity(); }

    /**
     * Reduce unused capacity on the bytes to fit the current size.
     * Does NOT change the bytes's size or contents, therefore we don't
     * consider it for a copy or undo operation.
     */
    inline void shrink_to_fit() { markAsUsed(); this->value_.shrink_to_fit(); }

    /// Clear the bytes.
    inline void clear() {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::vector<uint8_t>>(this->value_);
      markAsUsed(); this->value_.clear();
    }

    /**
     * Insert an element into the bytes.
     * @param pos The position to insert.
     * @param value The element to insert.
     * @return An iterator to the element that was inserted.
     */
    typename std::vector<uint8_t>::const_iterator insert(typename std::vector<uint8_t>::const_iterator pos, const uint8_t& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        this->undo_->emplace(BytesOp::INSERT, index, 1, std::vector<uint8_t>());
      }
      markAsUsed(); return this->value_.insert(pos, value);
    }

    /**
     * Insert an element into the bytes, using move.
     * @param pos The position to insert.
     * @param value The element to insert.
     * @return An iterator to the element that was inserted.
     */
    typename std::vector<uint8_t>::const_iterator insert(typename std::vector<uint8_t>::const_iterator pos, uint8_t&& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        this->undo_->emplace(BytesOp::INSERT, index, 1, std::vector<uint8_t>());
      }
      markAsUsed(); return this->value_.insert(pos, value);
    }

    /**
     * Insert a repeated number of the same element into the bytes.
     * @param pos The position to insert.
     * @param count The number of times to insert.
     * @param value The element to insert.
     * @return An iterator to the first element that was inserted.
     */
    typename std::vector<uint8_t>::const_iterator insert(typename std::vector<uint8_t>::const_iterator pos, std::size_t count, const uint8_t& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        this->undo_->emplace(BytesOp::INSERT_BULK, index, count, std::vector<uint8_t>());
      }
      markAsUsed(); return this->value_.insert(pos, count, value);
    }

    /**
     * Insert a range of elements into the bytes using iterators.
     * @param pos The position to insert.
     * @param first An iterator to the first value.
     * @param last An iterator to the last value.
     * @return An iterator to the first element that was inserted.
     */
    template<class InputIt> requires std::input_iterator<InputIt> typename std::vector<uint8_t>::const_iterator insert(
      typename std::vector<uint8_t>::const_iterator pos, InputIt first, InputIt last
    ) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        std::size_t diff = std::distance(first, last);
        this->undo_->emplace(BytesOp::INSERT_BULK, index, diff, std::vector<uint8_t>());
      }
      markAsUsed(); return this->value_.insert(pos, first, last);
    }

    /**
     * Insert a list of elements into the bytes.
     * @param pos The position to insert.
     * @param ilist The list of elements to insert.
     * @return An iterator to the first element that was inserted.
     */
    typename std::vector<uint8_t>::const_iterator insert(typename std::vector<uint8_t>::const_iterator pos, std::initializer_list<uint8_t> ilist) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        this->undo_->emplace(BytesOp::INSERT_BULK, index, ilist.size(), std::vector<uint8_t>());
      }
      markAsUsed(); return this->value_.insert(pos, ilist);
    }

    /**
     * Emplace (construct in-place) an element into the bytes.
     * @param pos The position to emplace.
     * @param args The element to emplace.
     * @return An iterator to the element that was emplaced.
     */
    template <class... Args> typename std::vector<uint8_t>::const_iterator emplace(typename std::vector<uint8_t>::const_iterator pos, Args&&... args) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(BytesOp::EMPLACE, std::distance(this->value_.cbegin(), pos), 1, std::vector<uint8_t>());
      }
      markAsUsed(); return this->value_.emplace(pos, args...);
    }

    /**
     * Erase an element from the bytes.
     * @param pos The index of the element to erase.
     * @return An iterator to the element after the removed one.
     */
    typename std::vector<uint8_t>::const_iterator erase(typename std::vector<uint8_t>::const_iterator pos) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), pos);
        this->undo_->emplace(BytesOp::ERASE, index, 1, std::vector<uint8_t>{this->value_.at(index)});
      }
      markAsUsed(); return this->value_.erase(pos);
    }

    /**
     * Erase a range of elements from the bytes.
     * @param first An iterator to the first value.
     * @param last An iterator to the last value.
     * @return An iterator to the element after the last removed one.
     */
    typename std::vector<uint8_t>::const_iterator erase(
      typename std::vector<uint8_t>::const_iterator first, typename std::vector<uint8_t>::const_iterator last
    ) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        std::size_t index = std::distance(this->value_.cbegin(), first);
        std::size_t diff = std::distance(first, last);
        auto oldVals = std::vector<uint8_t>(first, last);
        this->undo_->emplace(BytesOp::ERASE_BULK, index, diff, oldVals);
      }
      markAsUsed(); return this->value_.erase(first, last);
    }

    /**
     * Append an element to the end of the bytes.
     * @param value The value to append.
     */
    void push_back(const uint8_t& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(BytesOp::PUSH_BACK, 0, 0, std::vector<uint8_t>());
      }
      markAsUsed(); this->value_.push_back(value);
    }

    /**
     * Append an element to the end of the bytes, using move.
     * @param value The value to append.
     */
    void push_back(uint8_t&& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(BytesOp::PUSH_BACK, 0, 0, std::vector<uint8_t>());
      }
      markAsUsed(); this->value_.push_back(value);
    }

    /**
     * Emplace an element at the end of the bytes.
     * @param value The value to emplace.
     */
    uint8_t& emplace_back(uint8_t&& value) {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(BytesOp::EMPLACE_BACK, 0, 0, std::vector<uint8_t>());
      }
      markAsUsed(); return this->value_.emplace_back(value);
    }

    /// Erase the element at the end of the bytes.
    void pop_back() {
      if (this->copy_ == nullptr) {
        if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
        this->undo_->emplace(BytesOp::POP_BACK, 0, 0, std::vector<uint8_t>{this->value_.back()});
      }
      markAsUsed(); this->value_.pop_back();
    }

    /**
     * Resize the bytes to hold a given number of elements.
     * Default-constructed elements are appended.
     * @param count The number of items for the new size.
     */
    void resize(std::size_t count) {
      if (this->copy_ == nullptr) {
        if (count == 0) { // Treat as full operation if resize(0), otherwise treat as partial
          this->copy_ = std::make_unique<std::vector<uint8_t>>(this->value_);
        } else if (count != this->value_.size()) { // Only consider undo if size will actually change
          if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
          BytesOp bytesOp; // RESIZE_MORE (will be bigger) or RESIZE_LESS (will be smaller)
          std::size_t diff = 0; // Size difference between old and new vector
          std::vector<uint8_t> vals = {}; // Old values from before the operation
          if (count > this->value_.size()) {
            bytesOp = BytesOp::RESIZE_MORE;
            diff = count - this->value_.size();
          } else if (count < this->value_.size()) {
            bytesOp = BytesOp::RESIZE_LESS;
            diff = this->value_.size() - count;
            vals = std::vector<uint8_t>(this->value_.end() - diff, this->value_.end());
          }
          this->undo_->emplace(bytesOp, diff, 0, vals);
        }
      }
      markAsUsed(); this->value_.resize(count);
    }

    /**
     * Resize the bytes to hold a given number of elements.
     * If new size is bigger, new elements are appended and initialized.
     * @param count The number of items for the new size.
     * @param value The value to append and initialize.
     */
    void resize(std::size_t count, const uint8_t& value) {
      if (this->copy_ == nullptr) {
        if (count == 0) { // Treat as full operation if resize(0), otherwise treat as partial
          this->copy_ = std::make_unique<std::vector<uint8_t>>(this->value_);
        } else if (count != this->value_.size()) { // Only consider undo if size will actually change
          if (this->undo_ == nullptr) this->undo_ = std::make_unique<std::stack<UndoOp, std::vector<UndoOp>>>();
          BytesOp bytesOp; // RESIZE_MORE (will be bigger) or RESIZE_LESS (will be smaller)
          std::size_t diff = 0; // Size difference between old and new vector
          std::vector<uint8_t> vals = {}; // Old values from before the operation
          if (count > this->value_.size()) {
            bytesOp = BytesOp::RESIZE_MORE;
            diff = count - this->value_.size();
          } else if (count < this->value_.size()) {
            bytesOp = BytesOp::RESIZE_LESS;
            diff = this->value_.size() - count;
            vals = std::vector<uint8_t>(this->value_.end() - diff, this->value_.end());
          }
          this->undo_->emplace(bytesOp, diff, 0, vals);
        }
      }
      markAsUsed(); this->value_.resize(count, value);
    }

    ///@{
    /** Equality operator. Checks only the CURRENT value. */
    inline bool operator==(const std::vector<uint8_t>& other) const { return (this->value_ == other); }
    inline bool operator==(const SafeBytes& other) const { return (this->value_ == other.get()); }
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

#endif  // SAFEBYTES_H
