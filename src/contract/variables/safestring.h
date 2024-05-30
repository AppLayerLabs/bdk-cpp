/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFESTRING_H
#define SAFESTRING_H

#include <memory>
#include <string>
#include <string_view>

#include "safebase.h"

/**
 * Safe wrapper for a `std::string`. Used to safely store a string within a contract.
 * @see SafeBase
 */
class SafeString : public SafeBase {
  private:
    std::string value_; ///< Current ("original") value.
    std::unique_ptr<std::string> copy_; ///< Previous ("temporary") value.

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param str The initial value. Defaults to an empty string.
     */
    SafeString(DynamicContract *owner, const std::string& str = std::string())
      : SafeBase(owner), value_(str), copy_(nullptr)
    {};

    /// Empty constructor. Initializes an empty string.
    SafeString() : SafeBase(nullptr), value_(std::string()), copy_(nullptr) {}

    /**
     * Non-owning constructor.
     * @param str The initial value.
     */
    explicit SafeString(const std::string& str) : SafeBase(nullptr), value_(str), copy_(nullptr) {}

    /// Copy constructor. Only copies the CURRENT value.
    SafeString(const SafeString& other) : SafeBase(nullptr), value_(other.value_), copy_(nullptr) {}

    /// Getter for the CURRENT value.
    inline const std::string& get() const { return this->value_; }

    /**
     * Assign a new value from another string.
     * @param str The string to assign.
     * @return The new value.
     */
    inline SafeString& assign(const std::string& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.assign(str); return *this;
    }

    /**
     * Assign a new value from another string, using move
     * @param str The string to assign.
     * @return The new value.
     */
    inline SafeString& assign(std::string&& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.assign(std::move(str)); return *this;
    }

    /**
     * Assign a new value from the value of another SafeString.
     * @param str The string to use for replacement.
     * @return The new value.
     */
    inline SafeString& assign(const SafeString& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.assign(str.get()); return *this;
    }

    /**
     * Assign a new value from another substring.
     * @param str The string to use for replacement.
     * @param pos The position of the first character to be assigned.
     * @param count The number of characters of the substring to use.
     *              If the string itself is shorter, it will use as many
     *              characters as possible. Defaults to the end of the string.
     * @return The new value.
     */
    inline SafeString& assign(const std::string& str, size_t pos, size_t count = std::string::npos) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.assign(str, pos, count); return *this;
    }

    /**
     * Assign a new value from a substring of another SafeString.
     * @param str The string to use for replacement.
     * @param pos The position of the first character to be assigned.
     * @param count The number of characters of the substring to use.
     *              If the string itself is shorter, it will use as many
     *              characters as possible. Defaults to the end of the string.
     * @return The new value.
     */
    inline SafeString& assign(const SafeString& str, size_t pos, size_t count = std::string::npos) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.assign(str.get(), pos, count); return *this;
    }

    /**
     * Assign a new value from a number of chars.
     * @param count The number of characters to assign.
     * @param ch The character to fill the string with.
     * @return The new value.
     */
    inline SafeString& assign(size_t count, char ch) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.assign(count, ch); return *this;
    }

    /**
     * Assign a new value from a C-style string.
     * @param s The string to use for replacement.
     * @param count The number of characters to copy.
     * @return The new value.
     */
    inline SafeString& assign(const char* s, size_t count) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.assign(s, count); return *this;
    }

    /**
     * Assign a new value from a NULL-terminated C-style string.
     * @param s The string to use for replacement.
     * @return The new value.
     */
    inline SafeString& assign(const char* s) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.assign(s); return *this;
    }

    /**
     * Assign a new value from input iterators.
     * @tparam InputIt Any iterator type.
     * @param first An iterator pointing to the start of the input.
     * @param last An iterator pointing to the end of the input.
     * @return The new value.
     */
    template <class InputIt> inline SafeString& assign(InputIt first, InputIt last) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.assign(first, last); return *this;
    }

    /**
     * Assign a new value from an initializer list.
     * @param ilist The initializer list to use.
     * @return The new value.
     */
    inline SafeString& assign(std::initializer_list<char> ilist) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.assign(ilist); return *this;
    }

    ///@{
    /**
     * Get a character from a specified position.
     * @param pos The position of the character.
     * @return The requsted character.
     * @throws std::out_of_range if pos is bigger than the string's size.
     */
    inline char& at(size_t pos) {
      char& ret = this->value_.at(pos);
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return ret;
    }
    inline const char& at(size_t pos) const { return this->value_.at(pos); }
    ///@}

    ///@{
    /** Get the first character from the string. */
    inline char& front() {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.front();
    }
    inline const char& front() const { return this->value_.front(); }
    ///@}

    ///@{
    /** Get the last character from the string. */
    inline char& back() {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.back();
    }
    inline const char& back() const { return this->value_.back(); }
    ///@}

    /// Get the value from the pointer.
    inline const char* data() const { return this->value_.data(); }

    /// Same as data, but returns a NULL-terminated C-style string.
    inline const char* c_str() const { return this->value_.c_str(); }

    ///@{
    /** Get an iterator to the start of the string. */
    inline std::string::iterator begin() {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.begin();
    }
    inline std::string::const_iterator cbegin() const { return this->value_.cbegin(); }
    ///@}

    ///@{
    /** Get an iterator to the end of the string. */
    inline std::string::iterator end() {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.end();
    }
    inline std::string::const_iterator cend() const { return this->value_.cend(); }
    ///@}

    ///@{
    /** Get a reverse iterator to the start of a string. */
    inline std::string::reverse_iterator rbegin() {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.rbegin();
    }
    inline std::string::const_reverse_iterator crbegin() const { return this->value_.crbegin(); }
    ///@}

    ///@{
    /** Get a reverse iterator to the end of a string. */
    inline std::string::reverse_iterator rend() {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.rend();
    }
    inline std::string::const_reverse_iterator crend() const { return this->value_.crend(); }
    ///@}

    /**
     * Check if the string is empty (has no characters, aka "").
     * @return `true` if string is empty, `false` otherwise.
     */
    inline bool empty() const { return this->value_.empty(); }

    ///@{
    /** Get the number of characters in the string. */
    inline size_t size() const { return this->value_.size(); }
    inline size_t length() const { return this->value_.length(); }
    ///@}

    /// Get the maximum number of characters the string can hold.
    inline size_t max_size() const { return this->value_.max_size(); }

    /**
     * Increase the capacity of the string (how many characters it can hold).
     * @param newcap The new string capacity.
     */
    inline void reserve(size_t newcap) {
      if (this->copy_ == nullptr) {
        this->copy_ = std::make_unique<std::string>(this->value_);
        this->copy_->reserve(this->value_.capacity());
      }
      markAsUsed(); this->value_.reserve(newcap);
    }

    /// Get the number of characters that can be held in the currently allocated string.
    inline size_t capacity() const { return this->value_.capacity(); }

    /// Shrink the string to remove unused capacity.
    inline void shrink_to_fit() {
      if (this->copy_ == nullptr) {
        this->copy_ = std::make_unique<std::string>(this->value_);
        this->copy_->reserve(this->value_.capacity());
      }
      markAsUsed(); this->value_.shrink_to_fit();
    }

    /// Clear the contents of the string.
    inline void clear() {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.clear();
    }

    /**
     * Insert repeated characters into the string.
     * @param index The index at which the characters will be inserted.
     * @param count The number of characters to insert.
     * @param ch The character to insert.
     * @return The new value.
     */
    inline SafeString& insert(size_t index, size_t count, char ch) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.insert(index, count, ch); return *this;
    }

    /**
     * Insert a NULL-terminated C-style string into the string.
     * @param index The index at which the substring will be inserted.
     * @param s The substring to insert.
     * @return The new value.
     */
    inline SafeString& insert(size_t index, const char* s) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.insert(index, s); return *this;
    }

    /**
     * Insert a C-style substring into the string.
     * @param index The index at which the substring will be inserted.
     * @param s The substring to insert.
     * @param count The number of characters from the substring to insert.
     * @return The new value.
     */
    inline SafeString& insert(size_t index, const char* s, size_t count) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.insert(index, s, count); return *this;
    }

    /**
     * Insert a SafeString into the string.
     * @param index The index at which the substring will be inserted.
     * @param str The substring to insert.
     * @return The new value.
     */
    inline SafeString& insert(size_t index, const SafeString& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.insert(index, str.get()); return *this;
    }

    /**
     * Insert a string into the string (yes).
     * @param index The index at which the substring will be inserted.
     * @param str The substring to insert.
     * @return The new value.
     */
    inline SafeString& insert(size_t index, const std::string& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.insert(index, str); return *this;
    }

    /**
     * Insert a SafeString substring into the string (cat got your tongue?).
     * @param index The index at which the characters will be inserted.
     * @param str The substring to insert.
     * @param index_str The index of the first character of the substring to insert.
     * @param count The number of characters of the substring to insert. Defaults to the whole string.
     * @return The new value.
     */
    inline SafeString& insert(
      size_t index, const SafeString& str, size_t index_str, size_t count = std::string::npos
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.insert(index, str.get(), index_str, count); return *this;
    }

    /**
     * Insert a substring into the string.
     * @param index The index at which the characters will be inserted.
     * @param str The string to insert.
     * @param index_str The index of the first character of the substring to insert.
     * @param count The number of characters of the substring to insert. Defaults to the whole string.
     * @return The new value.
     */
    inline SafeString& insert(
      size_t index, const std::string& str, size_t index_str, size_t count = std::string::npos
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.insert(index, str, index_str, count); return *this;
    }

    /**
     * Insert a single character into the string using an iterator.
     * @param pos An iterator pointing to the position at which the character will be inserted.
     * @param ch The character to insert.
     * @return An iterator that points to the inserted character.
     */
    inline std::string::iterator insert(std::string::const_iterator pos, char ch) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.insert(pos, ch);
    }

    /**
     * Insert a number of characters into the string using an iterator.
     * @param pos An iterator pointing to the position at which the character will be inserted.
     * @param count The number of characters to insert.
     * @param ch The character to insert.
     * @return An iterator that points to the first inserted character.
     */
    inline std::string::iterator insert(std::string::const_iterator pos, size_t count, char ch) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.insert(pos, count, ch);
    }

    /**
     * Insert a range [first, last) of characters into the string.
     * @tparam InputIt Any iterator type.
     * @param pos The position at which the characters will be inserted.
     * @param first An iterator that points to the first character to insert.
     * @param last An iterator that points to one past the last character to insert.
     * @return An iterator that points to the first inserted character.
     */
    template <class InputIt> inline std::string::iterator insert(
      std::string::const_iterator pos, InputIt first, InputIt last
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.insert(pos, first, last);
    }

    /**
     * Insert characters from an initializer list into the string.
     * @param pos The position at which the characters will be inserted.
     * @param ilist The initializer list to insert.
     * @return An iterator that points to the first inserted character.
     */
    inline std::string::iterator insert(
      std::string::const_iterator pos, std::initializer_list<char> ilist
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.insert(pos, ilist);
    }

    /**
     * Erase a specific number of characters from the string.
     * @param index The index of the first character to erase.
     * @param count The number of characters to erase. Defaults to the whole string.
     * @return The new value.
     */
    inline SafeString& erase(size_t index = 0, size_t count = std::string::npos) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.erase(index, count); return *this;
    }

    /**
     * Erase a specific character from the string.
     * @param position An iterator that points to the character to be erased.
     * @return An iterator that points to the character immediately following the erased character.
     */
    inline std::string::iterator erase(std::string::const_iterator position) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.erase(position);
    }

    /**
     * Erase a range of [first, last) characters from the string.
     * @param first An iterator that points to the first character to be erased.
     * @param last An iterator that points to one past the last character to be erased.
     * @return An iterator that points to the character pointed to by last
     * prior to erasing, or end() if no such character exists.
     */
    inline std::string::iterator erase(
      std::string::const_iterator first, std::string::const_iterator last
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.erase(first, last);
    }

    /**
     * Append a character to the end of the string.
     * @param ch The character to append.
     */
    inline void push_back(char ch) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.push_back(ch);
    }

    /// Remove the last character from the string.
    inline void pop_back() {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.pop_back();
    }

    /**
     * Append a number of characters to the end of the string.
     * @param count The number of characters to append.
     * @param ch The character to append.
     * @return The new value.
     */
    inline SafeString& append(size_t count, char ch) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.append(count, ch); return *this;
    }

    /**
     * Append a SafeString to the end of the string.
     * @param str The string to append.
     * @return The new value.
     */
    inline SafeString& append(const SafeString& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.append(str.get()); return *this;
    }

    /**
     * Append a string to the end of the string.
     * @param str The string to append.
     * @return The new value.
     */
    inline SafeString& append(const std::string& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.append(str); return *this;
    }

    /**
     * Append a SafeString substring to the end of the string (Eminem approved).
     * @param str The substring to append.
     * @param pos The index of the first character to append.
     * @param count The number of characters to append. Defaults to the whole string.
     * @return The new value.
     */
    inline SafeString& append(
      const SafeString& str, size_t pos, size_t count = std::string::npos
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.append(str.get(), pos, count); return *this;
    }

    /**
     * Append a substring to the end of the string.
     * @param str The substring to append.
     * @param pos The index of the first character to append.
     * @param count The number of characters to append. Defaults to the whole string.
     * @return The new value.
     */
    inline SafeString& append(
      const std::string& str, size_t pos, size_t count = std::string::npos
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.append(str, pos, count); return *this;
    }

    /**
     * Append a C-style substring to the end of the string.
     * @param s A pointer to the character array to append.
     * @param count The number of characters to append.
     * @return The new value.
     */
    inline SafeString& append(const char* s, size_t count) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.append(s, count); return *this;
    }

    /**
     * Append a NULL-terminated C-style string to the end of the string.
     * @param s A pointer to the character array to append.
     * @return The new value.
     */
    inline SafeString& append(const char* s) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.append(s); return *this;
    }

    /**
     * Append a range of [first, last) characters to the end of the string.
     * @tparam InputIt Any iterator type.
     * @param first An iterator that points to the first character to append.
     * @param last An iterator that points to one past the last character to append.
     * @return The new value.
     */
    template <class InputIt> inline SafeString& append(InputIt first, InputIt last) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.append(first, last); return *this;
    }

    /**
     * Append characters from an initializer list to the end of a string.
     * @param ilist The initializer list to append.
     * @return The new value.
     */
    inline SafeString& append(std::initializer_list<char> ilist) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.append(ilist); return *this;
    }

    ///@{
    /**
     * Compare the string to another string.
     * @param str The string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(const std::string& str) const { return this->value_.compare(str); }
    inline int compare(const SafeString& str) const { return this->value_.compare(str.get()); }
    ///@}

    ///@{
    /**
     * Compare the string to another substring.
     * @param pos The index of the first character of the substring to compare to.
     * @param count The number of characters of the substring to compare to.
     * @param str The string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(size_t pos, size_t count, const std::string& str) const {
      return this->value_.compare(pos, count, str);
    }
    inline int compare(size_t pos, size_t count, const SafeString& str) const {
      return this->value_.compare(pos, count, str.get());
    }
    ///@}

    ///@{
    /**
     * Compare a substring of this string to another substring
     * (you better check yourself before you wreck yourself!).
     * @param pos1 The index of the first character of this string.
     * @param count1 The number of characters of the substring of this string.
     * @param str The substring to compare to.
     * @param pos2 The index of the first character of the substring to compare to.
     * @param count2 The number of characters of the substring to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(
      size_t pos1, size_t count1, const std::string& str,
      size_t pos2, size_t count2 = std::string::npos
    ) const {
      return this->value_.compare(pos1, count1, str, pos2, count2);
    }
    inline int compare(
      size_t pos1, size_t count1, const SafeString& str,
      size_t pos2, size_t count2 = std::string::npos
    ) const {
      return this->value_.compare(pos1, count1, str.get(), pos2, count2);
    }
    ///@}

    /**
     * Compare the string to another C-style string.
     * @param s The string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(const char* s) const { return this->value_.compare(s); }

    /**
     * Compare the string to another C-style substring.
     * @param pos The index of the first character of the substring to compare to.
     * @param count The number of characters of the substring to compare to.
     * @param s The string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(size_t pos, size_t count, const char* s) const {
      return this->value_.compare(pos, count, s);
    }

    /**
     * Compare a substring of this string to another C-style substring.
     * @param pos1 The index of the first character of this string.
     * @param count1 The number of characters of this substring.
     * @param s The string to compare to.
     * @param count2 The number of characters of the string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(size_t pos1, size_t count1, const char* s, size_t count2) const {
      return this->value_.compare(pos1, count1, s, count2);
    }

    /**
     * Check if the string starts with a given substring.
     * @param sv The substring to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool starts_with(std::string_view sv) const { return this->value_.starts_with(sv); }

    /**
     * Check if the string starts with a given character.
     * @param ch The character to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool starts_with(char ch) const { return this->value_.starts_with(ch); }

    /**
     * Check if the string starts with a given C-style substring.
     * @param s The substring to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool starts_with(const char* s) const { return this->value_.starts_with(s); }

    /**
     * Check if the string ends with a given substring.
     * @param sv The substring to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool ends_with(std::string_view sv) const { return this->value_.ends_with(sv); }

    /**
     * Check if the string ends with a given character.
     * @param ch The character to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool ends_with(char ch) const { return this->value_.ends_with(ch); }

    /**
     * Check if the string ends with a given C-style substring.
     * @param s The substring to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool ends_with(const char* s) const { return this->value_.ends_with(s); }

    /**
     * Check if the string contains a given substring.
     * @param sv The substring to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool contains(std::string_view sv) const { return this->value_.contains(sv); }

    /**
     * Check if the string contains a given character.
     * @param ch The character to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool contains(char ch) const { return this->value_.contains(ch); }

    /**
     * Check if the string contains a given C-style substring.
     * @param s The substring to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool contains(const char* s) const { return this->value_.contains(s); }

    ///@{
    /**
     * Replace part of this string with another string.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters of this string to replace.
     * @param str The string to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(size_t pos, size_t count, const SafeString& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(pos, count, str.get()); return *this;
    }
    inline SafeString& replace(size_t pos, size_t count, const std::string& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(pos, count, str); return *this;
    }
    ///@}

    ///@{
    /**
     * Replace part of this string with another string, using iterators.
     * @param first An iterator to the first character of this string to replace.
     * @param last An iterator to the last character of this string to replace.
     * @param str The string to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      std::string::const_iterator first, std::string::const_iterator last, const SafeString& str
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(first, last, str.get()); return *this;
    }
    inline SafeString& replace(
      std::string::const_iterator first, std::string::const_iterator last, const std::string& str
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(first, last, str); return *this;
    }
    ///@}

    ///@{
    /**
     * Replace part of this string with a substring.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters of this string to replace.
     * @param str The string to use as a replacement.
     * @param pos2 The index of the first character of the substring to use as a replacement.
     * @param count2 The number of characters of the substring to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      size_t pos, size_t count, const SafeString& str, size_t pos2, size_t count2 = std::string::npos
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(pos, count, str.get(), pos2, count2); return *this;
    }
    inline SafeString& replace(
      size_t pos, size_t count, const std::string& str, size_t pos2, size_t count2 = std::string::npos
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(pos, count, str, pos2, count2); return *this;
    }
    ///@}

    /**
     * Replace part of this string with a substring, using iterators.
     * @tparam InputIt Any iterator type.
     * @param first An iterator to the first character of this string to replace.
     * @param last An iterator to the last character of this string to replace.
     * @param first2 An iterator to the first character of the substring to use as a replacement.
     * @param last2 An iterator to the last character of the substring to use as a replacement.
     * @return The new value.
     */
    template <class InputIt> inline SafeString& replace(
      std::string::const_iterator first, std::string::const_iterator last,
      InputIt first2, InputIt last2
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(first, last, first2, last2); return *this;
    }

    /**
     * Replace part of this string with a C-style substring.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters in this string to replace.
     * @param cstr The array of characters to use as a replacement.
     * @param count2 The number of characters of the array to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(size_t pos, size_t count, const char* cstr, size_t count2) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(pos, count, cstr, count2); return *this;
    }

    /**
     * Replace part of this string with a C-style substring, using iterators.
     * @param first An iterator to the first character of this string to replace.
     * @param last An iterator to the last character of this string to replace.
     * @param cstr The array of characters to use as a replacement.
     * @param count The number of characters of the array to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      std::string::const_iterator first, std::string::const_iterator last,
      const char* cstr, size_t count
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(first, last, cstr, count); return *this;
    }

    /**
     * Replace part of this string with a C-style string.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters in this string to replace.
     * @param cstr The array of characters to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(size_t pos, size_t count, const char* cstr) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(pos, count, cstr); return *this;
    }

    /**
     * Replace part of this string with a C-style string, using iterators.
     * @param first An iterator to the first character of this string to replace.
     * @param last An iterator to the last character of this string to replace.
     * @param cstr The array of characters to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      std::string::const_iterator first, std::string::const_iterator last, const char* cstr
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(first, last, cstr); return *this;
    }

    /**
     * Replace part of this string with a number of repeated characters.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters in this string to replace.
     * @param count2 The number of characters to replace.
     * @param ch The character to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(size_t pos, size_t count, size_t count2, char ch) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(pos, count, count2, ch); return *this;
    }

    /**
     * Replace part of this string with a number of repeated characters, using iterators.
     * @param first An iterator to the first character of this string to replace.
     * @param last An iterator to the last character of this string to replace.
     * @param count The number of characters to replace.
     * @param ch The character to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      std::string::const_iterator first, std::string::const_iterator last,
      size_t count, char ch
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(first, last, count, ch); return *this;
    }

    /**
     * Replace part of this string with an initializer list of characters, using iterators.
     * @param first An iterator to the first character of this string to replace.
     * @param last An iterator to the last character of this string to replace.
     * @param ilist The initializer list to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      std::string::const_iterator first, std::string::const_iterator last,
      std::initializer_list<char> ilist
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(first, last, ilist); return *this;
    }

    /**
     * Replace part of this string with a string view.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters in this string to replace.
     * @param sv The string view to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      size_t pos, size_t count, const std::string_view& sv
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(pos, count, sv); return *this;
    }

    /**
     * Replace part of this string with a string view, using iterators.
     * @param first An iterator to the first character of this string to replace.
     * @param last An iterator to the last character of this string to replace.
     * @param sv The string view to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      std::string::const_iterator first, std::string::const_iterator last, const std::string_view& sv
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(first, last, sv); return *this;
    }

    /**
     * Replace part of this string with a string view.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters in this string to replace.
     * @param sv The string view to use as a replacement.
     * @param pos2 The index of the first character of the string view to use as a replacement.
     * @param count2 The number of characters of the string view to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      size_t pos, size_t count, const std::string_view& sv, size_t pos2, size_t count2 = std::string::npos
    ) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.replace(pos, count, sv, pos2, count2); return *this;
    }

    /**
     * Get a substring of the string.
     * @param pos The index of the first character of the substring.
     * @param count The number of characters of the substring.
     * @return The substring itself.
     */
    inline SafeString substr(size_t pos = 0, size_t count = std::string::npos) const {
      return SafeString(this->value_.substr(pos, count));
    }

    /**
     * Copy a substring of the string to a buffer.
     * @param dest The buffer to copy to.
     * @param count The number of characters to copy.
     * @param pos The index of the first character to copy.a
     * @return The number of characters that were copied.
     */
    inline size_t copy(char* dest, size_t count, size_t pos = 0) const {
      return this->value_.copy(dest, count, pos);
    }

    /**
     * Resize the string.
     * @param count The new size of the string.
     */
    inline void resize(size_t count) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.resize(count);
    }

    /**
     * Resize the string and fill the extra space with a given character.
     * @param count The new size of the string.
     * @param ch The character to use as filling.
     */
    inline void resize(size_t count, char ch) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.resize(count, ch);
    }

    /**
     * Swap the contents of this string with another string.
     * @param str The string to swap with.
     */
    inline void swap(std::string& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.swap(str);
    }

    /**
     * Swap the contents of this string with another SafeString.
     * @param other The string to swap with.
     */
    inline void swap(SafeString& other) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      if (other.copy_ == nullptr) other.copy_ = std::make_unique<std::string>(other.value_);
      markAsUsed(); other.markAsUsed(); this->value_.swap(other.value_);
    }

    ///@{
    /**
     * Find the first occurrence of a given string.
     * @param str The string to find.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find(const std::string& str, size_t pos = 0) const { return this->value_.find(str, pos); }
    inline size_t find(const SafeString& str, size_t pos = 0) const { return this->value_.find(str.get(), pos); }
    ///@}

    /**
     * Find the first occurrence of a given C-style substring.
     * @param s The string to find.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find(const char* s, size_t pos, size_t count) const { return this->value_.find(s, pos, count); }

    /**
     * Find the first occurrence of a given C-style string.
     * @param s The string to find.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find(const char* s, size_t pos = 0) const { return this->value_.find(s, pos); }

    /**
     * Find the first occurrence of a given character.
     * @param ch The character to find.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find(char ch, size_t pos = 0) const { return this->value_.find(ch, pos); }

    ///@{
    /**
     * Find the last occurrence of a given string.
     * @param str The string to find.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t rfind(const SafeString& str, size_t pos = std::string::npos) const {
      return this->value_.rfind(str.get(), pos);
    }
    inline size_t rfind(const std::string& str, size_t pos = std::string::npos) const {
      return this->value_.rfind(str, pos);
    }
    ///@}

    /**
     * Find the last occurrence of a given C-style substring.
     * @param s The string to find.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t rfind(const char* s, size_t pos, size_t count) const {
      return this->value_.rfind(s, pos, count);
    }

    /**
     * Find the last occurrence of a given C-style string.
     * @param s The string to find.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t rfind(const char* s, size_t pos = std::string::npos) const {
      return this->value_.rfind(s, pos);
    }

    /**
     * Find the last occurrence of a given character.
     * @param ch The character to find.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t rfind(char ch, size_t pos = std::string::npos) const {
      return this->value_.rfind(ch, pos);
    }

    ///@{
    /**
     * Find the first occurrence of any of the characters in a given string.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_of(const SafeString& str, size_t pos = 0) const {
      return this->value_.find_first_of(str.get(), pos);
    }
    inline size_t find_first_of(const std::string& str, size_t pos = 0) const {
      return this->value_.find_first_of(str, pos);
    }
    ///@}

    /**
     * Find the first occurrence of any of the characters in a given C-style substring.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_of(const char* s, size_t pos, size_t count) const {
      return this->value_.find_first_of(s, pos, count);
    }

    /**
     * Find the first occurrence of any of the characters in a given C-style string.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_of(const char* s, size_t pos = 0) const {
      return this->value_.find_first_of(s, pos);
    }

    /**
     * Find the first occurrence of a given character.
     * @param ch The character to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_of(char ch, size_t pos = 0) const {
      return this->value_.find_first_of(ch, pos);
    }

    ///@{
    /**
     * Find the first occurrence of none of the characters in a given string.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_not_of(const SafeString& str, size_t pos = 0) const {
      return this->value_.find_first_not_of(str.get(), pos);
    }
    inline size_t find_first_not_of(const std::string& str, size_t pos = 0) const {
      return this->value_.find_first_not_of(str, pos);
    }
    ///@}

    /**
     * Find the first occurrence of none of the characters in a given C-style substring.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_not_of(const char* s, size_t pos, size_t count) const {
      return this->value_.find_first_not_of(s, pos, count);
    }

    /**
     * Find the first occurrence of none of the characters in a given C-style string.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_not_of(const char* s, size_t pos = 0) const {
      return this->value_.find_first_not_of(s, pos);
    }

    /**
     * Find the first occurrence that's not the given character.
     * @param ch The character to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_not_of(char ch, size_t pos = 0) const {
      return this->value_.find_first_not_of(ch, pos);
    }

    ///@{
    /**
     * Find the last occurrence of any of the characters in a given string.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_of(const SafeString& str, size_t pos = std::string::npos) const {
      return this->value_.find_last_of(str.get(), pos);
    }
    inline size_t find_last_of(const std::string& str, size_t pos = std::string::npos) const {
      return this->value_.find_last_of(str, pos);
    }
    ///@}

    /**
     * Find the last occurrence of any of the characters in a given C-style substring.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_of(const char* s, size_t pos, size_t count) const {
      return this->value_.find_last_of(s, pos, count);
    }

    /**
     * Find the last occurrence of any of the characters in a given C-style string.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_of(const char* s, size_t pos = std::string::npos) const {
      return this->value_.find_last_of(s, pos);
    }

    /**
     * Find the last occurrence of a given character.
     * @param ch The character to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_of(char ch, size_t pos = std::string::npos) const {
      return this->value_.find_last_of(ch, pos);
    }

    ///@{
    /**
     * Find the last occurrence of none of the characters in a given string.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_not_of(const SafeString& str, size_t pos = std::string::npos) const {
      return this->value_.find_last_not_of(str.get(), pos);
    }
    inline size_t find_last_not_of(const std::string& str, size_t pos = std::string::npos) const {
      return this->value_.find_last_not_of(str, pos);
    }
    ///@}

    /**
     * Find the last occurrence of none of the characters in a given C-style substring.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_not_of(const char* s, size_t pos, size_t count) const {
      return this->value_.find_last_not_of(s, pos, count);
    }

    /**
     * Find the last occurrence of none of the characters in a given C-style string.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_not_of(const char* s, size_t pos = std::string::npos) const {
      return this->value_.find_last_not_of(s, pos);
    }

    /**
     * Find the last occurrence that's not the given character.
     * @param ch The character to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_not_of(char ch, size_t pos = std::string::npos) const {
      return this->value_.find_last_not_of(ch, pos);
    }

    ///@{
    /** Assignment operator. */
    inline SafeString& operator=(const SafeString& other) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_ = other.get(); return *this;
    }
    inline SafeString& operator=(const std::string& other) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_ = other; return *this;
    }
    inline SafeString& operator=(const char* s) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_ = s; return *this;
    }
    inline SafeString& operator=(char ch) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_ = ch; return *this;
    }
    inline SafeString& operator=(std::initializer_list<char> ilist) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_ = ilist; return *this;
    }
    ///@}

    ///@{
    /** Compound assignment operator. */
    inline SafeString& operator+=(const SafeString& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.operator+=(str.get()); return *this;
    }
    inline SafeString& operator+=(const std::string& str) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.operator+=(str); return *this;
    }
    inline SafeString& operator+=(const char* s) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.operator+=(s); return *this;
    }
    inline SafeString& operator+=(char ch) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.operator+=(ch); return *this;
    }
    inline SafeString& operator+=(std::initializer_list<char> ilist) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); this->value_.operator+=(ilist); return *this;
    }
    ///@}

    ///@{
    /** Subscript/Indexing operator. */
    inline char& operator[](size_t pos) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<std::string>(this->value_);
      markAsUsed(); return this->value_.operator[](pos);
    }
    inline const char& operator[](size_t pos) const { return this->value_.operator[](pos); }
    ///@}

    ///@{
    /** Concat operator. */
    inline SafeString operator+(const SafeString& rhs) const { return SafeString(this->value_ + rhs.get()); };
    inline SafeString operator+(const std::string& rhs) const { return SafeString(this->value_ + rhs); };
    inline SafeString operator+(const char* rhs) const { return SafeString(this->value_ + rhs); };
    inline SafeString operator+(char rhs) const { return SafeString(this->value_ + rhs); };
    ///@}

    ///@{
    /** Equality operator. */
    inline bool operator==(const SafeString& rhs) const { return this->value_ == rhs.get(); };
    inline bool operator==(const std::string& rhs) const { return this->value_ == rhs; };
    inline bool operator==(const char* rhs) const { return this->value_ == rhs; };
    ///@}

    ///@{
    /** Inequality operator. */
    inline bool operator!=(const SafeString& rhs) const { return this->value_ != rhs.get(); };
    inline bool operator!=(const std::string& rhs) const { return this->value_ != rhs; };
    inline bool operator!=(const char* rhs) const { return this->value_ != rhs; };
    ///@}

    ///@{
    /** Lesser comparison operator. */
    inline bool operator<(const SafeString& rhs) const { return this->value_ < rhs.get(); };
    inline bool operator<(const std::string& rhs) const { return this->value_ < rhs; };
    inline bool operator<(const char* rhs) const { return this->value_ < rhs; };
    ///@}

    ///@{
    /** Greater comparison operator. */
    inline bool operator>(const SafeString& rhs) const { return this->value_ > rhs.get(); };
    inline bool operator>(const std::string& rhs) const { return this->value_ > rhs; };
    inline bool operator>(const char* rhs) const { return this->value_ > rhs; };
    ///@}

    ///@{
    /** Lesser-or-equal comparison operator. */
    inline bool operator<=(const SafeString& rhs) const { return this->value_ <= rhs.get(); };
    inline bool operator<=(const std::string& rhs) const { return this->value_ <= rhs; };
    inline bool operator<=(const char* rhs) const { return this->value_ <= rhs; };
    ///@}

    ///@{
    /** Greater-or-equal comparison operator. */
    inline bool operator>=(const SafeString& rhs) const { return this->value_ >= rhs.get(); };
    inline bool operator>=(const std::string& rhs) const { return this->value_ >= rhs; };
    inline bool operator>=(const char* rhs) const { return this->value_ >= rhs; };
    ///@}

    /// Commit the value.
    inline void commit() override { this->copy_ = nullptr; this->registered_ = false; }

    /// Revert the value.
    inline void revert() override {
      if (this->copy_ != nullptr) {
        // Copying a string doesn't copy its capacity, we have to do it manually.
        // Same goes for reserve() and shrink_to_fit().
        // See https://stackoverflow.com/a/24399554 and https://stackoverflow.com/a/38785417
        if (this->copy_->capacity() > this->value_.capacity()) {
          this->value_.reserve(this->copy_->capacity());
          this->value_ = *this->copy_;
        } else if (this->copy_->capacity() < this->value_.capacity()) {
          this->value_ = *this->copy_;
          this->value_.shrink_to_fit();
        } else {
          this->value_ = *this->copy_;
        }
      }
      this->copy_ = nullptr; this->registered_ = false;
    }
};

/**
 * Overload of the bitwise <<operator for SafeStrings.
 * @param _out The output stream to write to.
 * @param _t The safe string to write.
 * @return The output stream.
*/
inline std::ostream& operator<<(std::ostream& _out, SafeString const& _t) { _out << _t.get(); return _out; }

#endif  // SAFESTRING_H
