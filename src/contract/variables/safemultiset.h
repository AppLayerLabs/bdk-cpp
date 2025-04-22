/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEMULTISET_H
#define SAFEMULTISET_H

#include <functional>
#include <set>
#include <utility>
#include "safebase.h"

// TODO: missing functions (not sure if necessary):
// merge, equal_range, operator==, operator<=>

/**
 * Safe wrapper for std::multiset.
 * @tparam Key Defines the type of the set key element.
 * @tparam Compare Defines the function object for performing comparisons.
 *                 Defaults to std::less (elements ordered from lower to higher).
 * @see SafeBase
 */
template <class Key, class Compare = std::less<Key>>  // TODO: Allocator? Do we need it?
class SafeMultiSet : public SafeBase {
  private:
    std::multiset<Key, Compare> value_; ///<  Current ("original") value.
    std::multiset<Key, Compare> copy_;  ///< Previous ("temporary") value.
  public:
    /// Default Constructor.
    SafeMultiSet() : SafeBase(nullptr) {};

    /**
     * Constructor with owner.
     * @param owner The owner of the variable.
     */
    SafeMultiSet(DynamicContract* owner) : SafeBase(owner) {};

    /**
     * Constructor with iterators.
     * @param first Iterator to the start of the data.
     * @param last Iterator to the end of the data.
     */
    template<class InputIt> SafeMultiSet(InputIt first, InputIt last) {
      this->value_.insert(first, last);
      this->copy_.insert(first, last);
    }

    /**
     * Constructor with initializer list.
     * @param init The initializer list.
     */
    SafeMultiSet(std::initializer_list<Key> init) {
      for (const auto& val : init) { this->value_.emplace(val); this->copy_.emplace(val); }
    }

    /// Copy constructor. only copies CURRENT value.
    SafeMultiSet(const SafeMultiSet& other) : value_(other.value_), copy_(other.value_) {
    }
    /// Return the value set const begin().
    inline typename std::multiset<Key>::iterator begin() { this->markAsUsed(); return this->value_.begin(); };

    /// Return the value set const end().
    inline typename std::multiset<Key>::iterator end() { this->markAsUsed(); return this->value_.end(); };

    /// Return the value set const crbegin().
    inline typename std::multiset<Key>::reverse_iterator rbegin() { this->markAsUsed(); return this->value_.rbegin(); };

    /// Return the value set const crend().
    inline typename std::multiset<Key>::reverse_iterator rend() { this->markAsUsed(); return this->value_.rend(); };

    /// Return the value set const begin().
    inline typename std::multiset<Key>::const_iterator cbegin() const { return this->value_.cbegin(); }

    /// Return the value set const end().
    inline typename std::multiset<Key>::const_iterator cend() const { return this->value_.cend(); }

    /// Return the value set const crbegin().
    inline typename std::multiset<Key>::const_reverse_iterator crbegin() const { return this->value_.crbegin(); }

    /// Return the value set const crend().
    inline typename std::multiset<Key>::const_reverse_iterator crend() const { return this->value_.crend(); }

    /// Check if value is empty.
    inline bool empty() const { return this->value_.empty(); }

    /// Get temporary set size.
    inline std::size_t size() const { return this->value_.size(); }

    /// Get temporary set max_size.
    inline std::size_t max_size() const { return this->value_.max_size(); }

    /// Clear sets.
    inline void clear() { this->markAsUsed(); this->value_.clear(); };

    /**
     * Insert an element into the set.
     * @param value The value to insert
     * @return An iterator to the inserted position.
     */
    typename std::multiset<Key>::iterator insert(const Key& value) {
      this->markAsUsed(); return this->value_.insert(value);
    }

    /// Move overload for insert().
    typename std::multiset<Key>::iterator insert(Key&& value) {
      this->markAsUsed(); return this->value_.insert(std::move(value));
    }

    /// Iterator overload for insert().
    typename std::multiset<Key>::iterator insert(
      typename std::multiset<Key>::const_iterator pos, const Key& value
    ) {
      this->markAsUsed(); return this->value_.insert(pos, value);
    }

    /// Iterator move overload for insert().
    typename std::multiset<Key>::iterator insert(
      typename std::multiset<Key>::const_iterator pos, Key&& value
    ) {
      this->markAsUsed(); return this->value_.insert(pos, std::move(value));
    }

    /// Range iterator overload for insert().
    template<class InputIt> void insert(InputIt first, InputIt last) {
      this->markAsUsed(); this->value_.insert(first, last);
    }

    /// Initializer list overload for insert().
    void insert(std::initializer_list<Key> ilist) {
      this->markAsUsed(); this->value_.insert(ilist);
    }

    /**
     * Construct elements inplace in the temporary set.
     * @param args The elements to insert.
     * @return An iterator to the last inserted element.
     */
    template<class... Args> typename std::multiset<Key>::iterator emplace(Args&&... args) {
      this->markAsUsed(); return this->value_.emplace(std::forward<Args>(args)...);
    }

    /**
     * Construct elements inplace using a hint.
     * @param hint The hint as to where to insert the elements.
     * @param args The elements to insert.
     * @return An iterator to the last inserted element.
n     */
    template<class... Args> typename std::multiset<Key>::iterator emplace_hint(
      typename std::multiset<Key>::const_iterator hint, Args&&... args
    ) {
      this->markAsUsed(); return this->value_.emplace_hint(hint, std::forward<Args>(args)...);
    }

    /**
     * Erase an element from the set.
     * @param pos An iterator to the element to be erased.
     * @return An iterator to the first element following the erased one.
     */
    typename std::multiset<Key>::const_iterator erase(typename std::multiset<Key>::const_iterator pos) {
      this->markAsUsed(); return this->value_.erase(pos);
    }

    /// Ranged overload of erase(). Erases [ first, last ) .
    typename std::multiset<Key>::const_iterator erase(
      typename std::multiset<Key>::const_iterator first, typename std::multiset<Key>::const_iterator last
    ) {
      this->markAsUsed(); return this->value_.erase(first, last);
    }

    /// Element-specific overload of erase(). Returns the number of erased elements.
    size_t erase(const Key& key) {
      this->markAsUsed(); return this->value_.erase(key);
    }

    /**
     * Swap elements with another set. Swaps only elements from the TEMPORARY sets.
     * @param other The set to swap with.
     */
    void swap(SafeMultiSet& other) {
      this->markAsUsed(); this->value_.swap(other.value_);
    }

    /**
     * Extract an element from the temporary set. Get the value itself with `.value()`.
     * @param pos An iterator to the element to be extracted.
     * @return The extracted element.
     */
    typename std::multiset<Key>::node_type extract(typename std::multiset<Key>::iterator pos) {
      this->markAsUsed(); return this->value_.extract(pos);
    }

    /// Element-specific overload of extract(), copy-wise.
    typename std::multiset<Key>::node_type extract(const Key& x) {
      this->markAsUsed(); return this->value_.extract(x);
    }

    /// Element-specific overload of extract(), move-wise.
    typename std::multiset<Key>::node_type extract(Key&& x) {
      this->markAsUsed(); return this->value_.extract(std::move(x));
    }

    /**
     * Count the number of elements that exist in the set.
     * @param key The key value to count.
     * @return The number of found elements.
     */
    size_t count(const Key& key) const { return this->value_.count(key); }

    /**
     * Find an element in the temporary set.
     * @param key The key to search for.
     * @return An iterator to the found element.
     */
    typename std::multiset<Key>::iterator find(const Key& key) {
      this->markAsUsed(); return this->value_.find(key);
    }


    /**
     * Find an element in the temporary set. (CONST OVERLOAD)
     * @param key The key to search for.
     * @return An iterator to the found element.
     */
    typename std::multiset<Key>::const_iterator find(const Key& key) const {
      return this->value_.find(key);
    }

    /**
     * Check if the set contains a given element.
     * @param key The key to check.
     * @return `true` if set contains the key, `false` otherwise.
     */
    bool contains(const Key& key) const { return this->value_.contains(key); }

    /**
     * Get the first element that is not less than the given one.
     * @param key The key to use for comparison.
     * @return An iterator to the found element.
     */
    typename std::multiset<Key>::const_iterator lower_bound(const Key& key) {
      return this->value_.lower_bound(key);
    }

    /**
     * Get the first element that is greater than the given one.
     * @param key The key to use for comparison.
     * @return An iterator to the found element.
     */
    typename std::multiset<Key>::const_iterator upper_bound(const Key& key) {
      return this->value_.upper_bound(key);
    }

    /// Get the function that compares the keys (same as value_comp).
    typename std::multiset<Key>::key_compare key_comp() const {
      return this->value_.key_comp();
    }

    /// Get the function that compares the values (same as key_comp).
    typename std::multiset<Key>::value_compare value_comp() const {
      return this->value_.value_comp();
    }

    /**
     * Erase all elements that satisfy the predicate from the container.
     * @param pred The predicate that returns `true` if the element should be erased.
     * @return The number of erased elements.
     */
    template<class Pred> size_t erase_if(Pred pred) {
      this->markAsUsed();
      size_t oldSize = this->value_.size();
      for (auto it = this->value_.begin(); it != this->value_.end();) {
        if (pred(*it)) {
          it = this->value_.erase(it);
        } else {
          ++it;
        }
      }
      return oldSize - this->value_.size();
    }

    /// Commit function.
    void commit() override {
      this->copy_ = this->value_;
      this->registered_ = false;
    }

    /// Rollback function.
    void revert() override {
      this->value_ = this->copy_;
      this->registered_ = false;
    }

    /// Get the inner set (for const functions!)
    inline const std::multiset<Key>& get() const { return this->value_; }
};

#endif  // SAFEMULTISET_H
