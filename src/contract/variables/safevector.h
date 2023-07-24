#ifndef SAFEVECTOR_H
#define SAFEVECTOR_H

#include <vector>
#include <map>
#include "safebase.h"

/**
 * Safe wrapper for `std::vector`.
 * This class employs a `std::map` for temporary storage of changes to the vector,
 * ensuring efficient memory usage without needing a full vector copy or
 * initializing an entire vector of `nullptr`s for each access.
 * `std::map` is preferred over `std::unordered_map` due to its inherent ordering.
 * This allows safe and efficient access to indices within the current size of the vector.
 * Additionally, ordered iteration over newly accessed keys and prior keys is required.
 * For instance, with a vector of size 10, accessing indices 3, 5, 7, 10, etc.
 * should allow for sequential iteration over those exact indices when committing.
 * Trying to access elements out of bounds will throw an exception.
 * @tparam T Defines the type of the vector elements.
 * @see SafeBase
 */
template <typename T> class SafeVector : public SafeBase {
  private:
    std::vector<T> vector_;                               ///< Original vector.
    mutable std::unique_ptr<std::map<uint64_t, T>> tmp_;  ///< Temporary map.
    mutable uint64_t maxIndex_ = 0;                       ///< Maximum index of the vector.
    mutable bool clear_ = false;                          ///< Whether the vector should be cleared.

    /// Check if the temporary map is initialized (and initialize if it not).
    inline void check() const {
      if (this->tmp_ == nullptr) {
        this->tmp_ = std::make_unique<std::map<uint64_t, T>>();
        this->maxIndex_ = this->vector_.size();
      }
    }

    /**
     * Check if a specific index exists in the temporary map, copying it from the original vector if not.
     * @param index The index to check.
     * @throw std::out_of_range if index is bigger than the maximum index of the vector.
     */
    inline void checkIndexAndCopy(const uint64_t& index) {
      this->check();
      if (index >= this->maxIndex_) throw std::out_of_range("Index out of range");
      if (this->tmp_->contains(index)) return;
      this->tmp_->emplace(index, this->vector_[index]);
    }

  public:
    /// Default Ccnstructor.
    SafeVector() : SafeBase(nullptr) {};

    /**
     * Constructor with owner.
     * @param owner The owner of the variable.
     */
    SafeVector(DynamicContract* owner) : SafeBase(owner) {};

    /**
     * Constructor that fills a specific number of indices with the given value.
     * @param count The number of indices to fill.
     * @param value The value to initialize.
     */
    SafeVector(std::size_t count, const T& value) {
      check();
      for (std::size_t i = 0; i < count; i++) {
        this->tmp_->emplace(i, value);
        this->maxIndex_++;
      }
    }

    /**
     * Constructor that fills a specific number of indices with empty values.
     * @param count The number of indices to fill.
     */
    explicit SafeVector(std::size_t count) {
      check();
      for (std::size_t i = 0; i < count; i++) {
        this->tmp_->emplace(i, T());
        this->maxIndex_++;
      }
    }

    /**
     * Constructor that fills indices using iterators.
     * @tparam InputIt The type of iterator to use.
     * @param first Iterator to the beginning of the type vector.
     * @param last Iterator to the end of the type vector.
     */
    template <class InputIt> SafeVector(InputIt first, InputIt last) {
      check();
      uint64_t i = 0;
      for (auto it = first; it != last; it++, i++) {
        this->tmp_->emplace(i, *it);
        this->maxIndex_++;
      }
    }

    /// Copy constructor.
    SafeVector(const SafeVector& other) {
      check();
      other.check();
      *this->tmp_ = *(other.tmp_);
      this->maxIndex_ = other.maxIndex_;
    }

    /**
     * Constructor that fills indices using an initializer list.
     * @param init The list to use.
     */
    SafeVector(std::initializer_list<T> init) {
      check();
      for (const auto& val : init) {
        this->tmp_->emplace(this->maxIndex_, val);
        this->maxIndex_++;
      }
    }

    /**
     * Replace the contents of the temporary map with a number of copies of a given value.
     * @param count The number of indices to replace.
     * @param value The value to use as a replacement.
     */
    inline void assign(std::size_t count, const T& value) {
      check();
      this->tmp_->clear();
      for (std::size_t i = 0; i < count; i++) this->tmp_->emplace(i, value);
      this->maxIndex_ = count;
      this->clear_ = true;
    }

    /**
     * Replace the contents of the temporary map with elements from the input range [first, last).
     * @tparam InputIt The type of iterator to use.
     * @param first Iterator to the beginning of the type vector.
     * @param last Iterator to the end of the type vector.
     */
    template<class InputIt> inline void assign(InputIt first, InputIt last) {
      check();
      this->tmp_->clear();
      uint64_t i = 0;
      for (auto it = first; it != last; it++, i++) this->tmp_->emplace(i, *it);
      this->maxIndex_ = i;
      this->clear_ = true;
    }

    /**
     * Replace the contents of the temporary map with elements from an initializer list.
     * @param ilist The list to use.
     */
    inline void assign(std::initializer_list<T> ilist) {
      check();
      this->tmp_->clear();
      uint64_t i = 0;
      for (const auto& val : ilist) {
        this->tmp_->emplace(i, val);
        i++;
      }
      this->maxIndex_ = i;
      this->clear_ = true;
    }

    /**
     * Access a specified element of the temporary map with bounds checking.
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    inline T& at(std::size_t pos) {
      checkIndexAndCopy(pos);
      markAsUsed();
      return this->tmp_->at(pos);
    }

    /**
     * Const overload of at().
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    const T& at(std::size_t pos) const {
      checkIndexAndCopy(pos);
      return this->tmp_->at(pos);
    }

    /**
     * Access a specified element of the temporary map without bounds checking.
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    inline T& operator[](std::size_t pos) {
      checkIndexAndCopy(pos);
      markAsUsed();
      return (*this->tmp_)[pos];
    }

    /**
     * Const overload of operator[].
     * @param pos The position of the index to access.
     * @return The element at the given index.
     */
    inline const T& operator[](std::size_t pos) const {
      checkIndexAndCopy(pos);
      return (*this->tmp_)[pos];
    }

    /// Get an iterator to the beginning of the original vector.
    inline std::vector<T>::const_iterator cbegin() const { return this->vector_.cbegin(); }

    /// Get an iterator to the end of the original vector.
    inline std::vector<T>::const_iterator cend() const { return this->vector_.cend(); }

    /// Get a reverse iterator to the beginning of the original vector.
    inline std::vector<T>::const_reverse_iterator crbegin() const { return this->vector_.crbegin(); }

    /// Get a reverse iterator to the end of the original vector.
    inline std::vector<T>::const_reverse_iterator crend() const { return this->vector_.crend(); }

    /**
     * Check if the original vector is empty (has no elements).
     * @return `true` if vector is empty, `false` otherwise.
     */
    inline bool empty() const { return (this->maxIndex_ == 0); }

    /**
     * Get the current size of the original vector.
     * @return The size of the vector.
     */
    inline std::size_t size() const { check(); return this->maxIndex_; }

    /**
     * Get the maximum possible size of the original vector.
     * @return The maximum size.
     */
    inline std::size_t max_size() const { return std::numeric_limits<size_t>::max() - 1; }

    /// Clear the temporary map.
    inline void clear() {
      check();
      markAsUsed();
      this->tmp_->clear();
      this->maxIndex_ = 0;
      this->clear_ = true;
    }

    /**
     * Insert an element into the temporary map.
     * This is not directly 1:1 to std::vector, as the temporary map can't
     * return an iterator to it (it is a std::map after all), so we use a
     * uint64_t as the key for the index of the inserted element.
     * @param pos The position of the index to insert the value.
     * @param value The value to insert.
     * @return The index of the inserted element.
     * @throw std::out_of_range if pos is bigger than the maximum size of the vector.
     */
    uint64_t insert(const uint64_t& pos, const T& value) {
      check();
      markAsUsed();
      if (pos == this->maxIndex_) {
        this->tmp_->insert_or_assign(pos, value);
        this->maxIndex_++;
        return pos;
      } else if (pos < this->maxIndex_) {
        // Move all elements from pos to maxIndex_ one position to the right, so we can fit the new element at pos.
        for (uint64_t i = this->maxIndex_; i > pos; i--) {
          auto iter = this->tmp_->find(i - 1);
          this->tmp_->insert_or_assign(i, (iter != this->tmp_->end())
            ? iter->second          // Shift the element from the iterator if the element was found there
            : this->vector_[i - 1]   // Copy and shift the element from the original vector if not
          );
        }
        this->tmp_->insert_or_assign(pos, value);
        this->maxIndex_++;
        return pos;
      } else {
        throw std::out_of_range("pos out of range");
      }
    }

    /**
     * Erase an element from the temporary map.
     * @param pos The position of the index to erase.
     * @return The index of the first element following the removed one.
     */
    std::size_t erase(std::size_t pos) {
      checkIndexAndCopy(pos);
      markAsUsed();
      // Shift elements from the right of pos to fill the gap.
      for (std::size_t i = pos; i < this->maxIndex_ - 1; i++) {
        auto iter = this->tmp_->find(i + 1);
        this->tmp_->insert_or_assign(i, (iter != this->tmp_->end())
          ? iter->second          // Shift the element from the iterator if the element was found there
          : this->vector_[i + 1]   // Copy and shift the element from the original vector if not
        );
      }
      // Remove the last element.
      this->tmp_->erase(this->maxIndex_ - 1);
      this->maxIndex_--;
      return pos;
    }

    /**
     * Erase a range of elements from the temporary map.
     * @param first The first index position to erase.
     * @param last The last index position to erase.
     * @return The index of the first element following the removed ones.
     * @throw std::out_of_range if first is bigger thanlast, or last is bigger than the maximum size of the vector.
     */
    std::size_t erase(std::size_t first, std::size_t last) {
      check();
      markAsUsed();
      if (first > last || last > this->maxIndex_) throw std::out_of_range("Indices out of range");
      // Compute the number of elements to be removed.
      std::size_t numToRemove = last - first;
      // Shift elements from the right of last to fill the gap.
      for (std::size_t i = first; i < this->maxIndex_ - numToRemove; ++i) {
        auto iter = this->tmp_->find(i + numToRemove);
        this->tmp_->insert_or_assign(i, (iter != this->tmp_->end())
          ? iter->second                    // Shift the element from the iterator if the element was found there
          : this->vector_[i + numToRemove]   // Copy and shift the element from the original vector if not
        );
      }
      // Remove the last numToRemove elements.
      for (std::size_t i = 0; i < numToRemove; i++) this->tmp_->erase(this->maxIndex_ - 1 - i);
      this->maxIndex_ -= numToRemove;
      return first;
    }

    /**
     * Append (copy) an element to the end of the temporary map.
     * @param value The value to append.
     */
    void push_back(const T& value) {
      check();
      markAsUsed();
      this->tmp_->emplace(this->maxIndex_, value);
      this->maxIndex_++;
    }

    /**
     * Emplace (move) an element to the end of the temporary map.
     * @param value The value to append.
     */
    void emplace_back(T&& value) {
      check();
      markAsUsed();
      this->tmp_->emplace(this->maxIndex_, std::move(value));
      this->maxIndex_++;
    }

    /// Remove the last element of the temporary map.
    void pop_back() {
      check();
      markAsUsed();
      this->tmp_->erase(this->maxIndex_ - 1);
      this->maxIndex_--;
    }

    /**
     * Resize the temporary map to a given new size.
     * If the new size is greater, default-constructed elements are appended.
     * If the new size is smaller, extra elements are erased.
     * @param count The number of items the temporary map should hold.
     */
    void resize(std::size_t count) {
      check();
      if (count < this->maxIndex_) {
        for (std::size_t i = count; i < this->maxIndex_; i++) this->tmp_->erase(i);
      } else if (count > this->maxIndex_) {
        for (std::size_t i = this->maxIndex_; i < count; i++) this->tmp_->emplace(i, T());
      }
      this->maxIndex_ = count;
      markAsUsed();
    }

    /// Changes the number of elements stored (new elements are appended and initialized with `value`)
    /**
     * Overload of resize() that replaces new default-constructed elements with a given predefined value.
     * @param count The number of items the temporary map should hold.
     * @param value The value to fill extra indices with, if necessary.
     */
    void resize(std::size_t count, const T& value) {
      check();
      if (count < this->maxIndex_) {
        for (std::size_t i = count; i < this->maxIndex_; i++) this->tmp_->erase(i);
      } else if (count > this->maxIndex_) {
        for (std::size_t i = this->maxIndex_; i < count; i++) this->tmp_->emplace(i, value);
      }
      this->maxIndex_ = count;
      markAsUsed();
    }

    /**
     * Commit the value.
     * Updates the original vector from the temporary map and clears it, or,
     * clears the original vector if the temporary map is set to empty.
     */
    void commit() override {
      check();
      if (this->clear_) { this->vector_.clear(); this->clear_ = false; }

      // Erase difference in size.
      if (this->vector_.size() > this->maxIndex_) {
        this->vector_.erase(this->vector_.begin() + this->maxIndex_, this->vector_.end());
      }

      for (auto& it : *this->tmp_) {
        if (it.first < this->vector_.size()) {
          this->vector_[it.first] = it.second;
        } else {
          this->vector_.emplace_back(it.second);
        }
      }
      this->maxIndex_ = this->vector_.size();
    }

    /// Revert the value. Nullifies the temporary map.
    void revert() const override {
      this->tmp_ = nullptr;
      this->clear_ = false;
      this->maxIndex_ = this->vector_.size();
    }
};

#endif // SAFEVECTOR_H
