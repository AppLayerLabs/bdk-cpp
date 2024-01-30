/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFESTRING_H
#define SAFESTRING_H

#include <memory>
#include <string>

#include "safebase.h"

/**
 * Safe wrapper for a string variable.
 * Used to safely store a string within a contract.
 * @see SafeBase
 */
class SafeString : SafeBase {
  private:
    std::string str_;  ///< Value.
    mutable std::unique_ptr<std::string> strPtr_; ///< Pointer to the value. check() requires this to be mutable.

    /// Check if the pointer is initialized (and initialize it if not).
    void check() const override {
      if (strPtr_ == nullptr) strPtr_ = std::make_unique<std::string>(str_);
    }

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param str The initial value. Defaults to an empty string.
     */
    SafeString(DynamicContract *owner, const std::string& str = std::string())
      : SafeBase(owner), strPtr_(std::make_unique<std::string>(str))
    {};

    /// Empty constructor. Initializes an empty string.
    SafeString() : SafeBase(nullptr), strPtr_(std::make_unique<std::string>()) {};

    /**
     * Non-owning constructor.
     * @param str The string initial value.
     */
    explicit SafeString(const std::string& str)
      : SafeBase(nullptr), strPtr_(std::make_unique<std::string>(str))
    {};

    /// Copy constructor.
    SafeString(const SafeString& other) : SafeBase(nullptr) {
      other.check(); strPtr_ = std::make_unique<std::string>(*other.strPtr_);
    }

    /// Getter for the value. Returns the value from the pointer.
    inline const std::string& get() const { check(); return *strPtr_; }

    /**
     * Commit the value. Updates the value from the pointer, nullifies it and
     * unregisters the variable.
     */
    inline void commit() override { check(); str_ = *strPtr_; registered_ = false; }

    /// Revert the value. Nullifies the pointer and unregisters the variable.
    inline void revert() const override { strPtr_ = nullptr; registered_ = false; }

    /**
     * Assign a new value from a number of chars.
     * @param count The number of characters to assign.
     * @param ch The character to fill the string with.
     * @return The new value.
     */
    inline SafeString& assign(size_t count, char ch) {
      check();
      markAsUsed();
      strPtr_->assign(count, ch);
      return *this;
    }

    /**
     * Assign a new value from the value of another SafeString.
     * @param str The string to use for replacement.
     * @return The new value.
     */
    inline SafeString& assign(const SafeString& str) {
      check();
      markAsUsed();
      strPtr_->assign(str.get());
      return *this;
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
      check();
      markAsUsed();
      strPtr_->assign(str.get(), pos, count);
      return *this;
    }

    /**
     * Assign a new value from a C-style string.
     * @param s The string to use for replacement.
     * @param count The number of characters to copy.
     * @return The new value.
     */
    inline SafeString& assign(const char* s, size_t count) {
      check();
      markAsUsed();
      strPtr_->assign(s, count);
      return *this;
    }

    /**
     * Assign a new value from a NULL-terminated C-style string.
     * @param s The string to use for replacement.
     * @return The new value.
     */
    inline SafeString& assign(const char* s) {
      check();
      markAsUsed();
      strPtr_->assign(s);
      return *this;
    }

    /**
     * Assign a new value from input iterators.
     * @param first An iterator pointing to the start of the input.
     * @param last An iterator pointing to the end of the input.
     * @return The new value.
     */
    template <class InputIt> inline SafeString& assign(InputIt first, InputIt last) {
      check();
      markAsUsed();
      strPtr_->assign(first, last);
      return *this;
    }

    /**
     * Assign a new value from an initializer list.
     * @param ilist The initializer list to use.
     * @return The new value.
     */
    inline SafeString& assign(std::initializer_list<char> ilist) {
      check();
      markAsUsed();
      strPtr_->assign(ilist);
      return *this;
    }

    /**
     * Get a character from a specified position.
     * @param pos The position of the character.
     * @return The requsted character.
     */
    inline char& at(size_t pos) { check(); markAsUsed(); return strPtr_->at(pos); }

    /// Const overload for at().
    inline const char& at(size_t pos) const { check(); return strPtr_->at(pos); }

    /// Get the first character from the string.
    inline char& front() { check(); markAsUsed(); return strPtr_->front(); }

    /// Const overload for front().
    inline const char& front() const { check(); return strPtr_->front(); }

    /// Get the last character from the string.
    inline char& back() { check(); markAsUsed(); return strPtr_->back(); }

    /// Const overload for back().
    inline const char& back() const { check(); return strPtr_->back(); }

    /// Get the value from the pointer as a NULL-terminated C-style string.
    inline const char* c_str() const { check(); return strPtr_->c_str(); }

    /// Get the value from the pointer.
    inline const char* data() const { check(); return strPtr_->data(); }

    /// Get an iterator to the start of the string.
    inline std::string::iterator begin() { check(); markAsUsed(); return strPtr_->begin(); }

    /// Get a const iterator to the start of the string.
    inline std::string::const_iterator cbegin() const { check(); return strPtr_->cbegin(); }

    /// Get an iterator to the end of the string.
    inline std::string::iterator end() { check(); markAsUsed(); return strPtr_->end(); }

    /// Get a const iterator to the end of the string.
    inline std::string::const_iterator cend() const { check(); return strPtr_->cend(); }

    /// Get a reverse iterator to the start of a string.
    inline std::string::reverse_iterator rbegin() { check(); markAsUsed(); return strPtr_->rbegin(); }

    /// Get a const reverse iterator to the start of a string.
    inline std::string::const_reverse_iterator crbegin() { check(); return strPtr_->crbegin(); }

    /// Get a reverse iterator to the end of a string.
    inline std::string::reverse_iterator rend() { check(); markAsUsed(); return strPtr_->rend(); }

    /// Get a const reverse iterator to the end of a string.
    inline std::string::const_reverse_iterator crend() { check(); return strPtr_->crend(); }

    /**
     * Check if the string is empty (has no characters, aka "").
     * @return `true` if string is empty, `false` otherwise.
     */
    inline bool empty() const { check(); return strPtr_->empty(); }

    /// Get the number of characters in the string.
    inline size_t size() const { check(); return strPtr_->size(); }

    /// Same as size().
    inline size_t length() const { check(); return strPtr_->length(); }

    /// Get the maximum number of characters the string can hold
    inline size_t max_size() const { check(); return strPtr_->max_size(); }

    /**
     * Increase the capacity of the string (how many characters it can hold).
     * @param newcap The new string capacity.
     */
    inline void reserve(size_t newcap) { check(); markAsUsed(); strPtr_->reserve(newcap); }

    /// Get the number of characters that can be held in the currently allocated string.
    inline size_t capacity() const { check(); return strPtr_->capacity(); }

    /// Shrink the string to remove unused capacity.
    inline void shrink_to_fit() { check(); markAsUsed(); strPtr_->shrink_to_fit(); }

    /// Clear the contents of the string.
    inline void clear() { check(); markAsUsed(); strPtr_->clear(); }

    /**
     * Insert characters into the string.
     * @param index The index at which the characters will be inserted.
     * @param count The number of characters to insert.
     * @param ch The character to insert.
     * @return The new value.
     */
    inline SafeString& insert(size_t index, size_t count, char ch) {
      check(); markAsUsed(); strPtr_->insert(index, count, ch); return *this;
    }

    /**
     * Insert a NULL_terminated C-style string into the string.
     * @param index The index at which the substring will be inserted.
     * @param s The substring to insert.
     * @return The new value.
     */
    inline SafeString& insert(size_t index, const char* s) {
      check(); markAsUsed(); strPtr_->insert(index, s); return *this;
    }

    /**
     * Insert a C-style substring into the string.
     * @param index The index at which the substring will be inserted.
     * @param s The substring to insert.
     * @param count The number of characters from the substring to insert.
     * @return The new value.
     */
    inline SafeString& insert(size_t index, const char* s, size_t count) {
      check(); markAsUsed(); strPtr_->insert(index, s, count); return *this;
    }

    /**
     * Insert a SafeString into the string.
     * @param index The index at which the substring will be inserted.
     * @param str The substring to insert.
     * @return The new value.
     */
    inline SafeString& insert(size_t index, const SafeString& str) {
      check(); markAsUsed(); strPtr_->insert(index, str.get()); return *this;
    }

    /**
     * Insert a string into the string (yes).
     * @param index The index at which the substring will be inserted.
     * @param str The substring to insert.
     * @return The new value.
     */
    inline SafeString& insert(size_t index, const std::string& str) {
      check(); markAsUsed(); strPtr_->insert(index, str); return *this;
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
      check(); markAsUsed(); strPtr_->insert(index, str.get(), index_str, count); return *this;
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
      check(); markAsUsed(); strPtr_->insert(index, str, index_str, count); return *this;
    }

    /**
     * Insert a single character into the string using an iterator.
     * @param pos An iterator pointing to the position at which the character will be inserted.
     * @param ch The character to insert.
     * @return An iterator that points to the inserted character.
     */
    inline std::string::iterator insert(std::string::const_iterator pos, char ch) {
      check(); markAsUsed(); return strPtr_->insert(pos, ch);
    }

    /**
     * Insert a number of characters into the string using an iterator.
     * @param pos An iterator pointing to the position at which the character will be inserted.
     * @param count The number of characters to insert.
     * @param ch The character to insert.
     * @return An iterator that points to the first inserted character.
     */
    inline std::string::iterator insert(std::string::const_iterator pos, size_t count, char ch) {
      check(); markAsUsed(); return strPtr_->insert(pos, count, ch);
    }

    /**
     * Insert a range [first, last) of characters into the string.
     * @param pos The position at which the characters will be inserted.
     * @param first An iterator that points to the first character to insert.
     * @param last An iterator that points to one past the last character to insert.
     * @return An iterator that points to the first inserted character.
     */
    template <class InputIt> inline std::string::iterator insert(
      std::string::const_iterator pos, InputIt first, InputIt last
    ) {
      check(); markAsUsed(); return strPtr_->insert(pos, first, last);
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
      check(); markAsUsed(); return strPtr_->insert(pos, ilist);
    }

    /**
     * Erase a specific number of characters from the string.
     * @param index The index of the first character to erase.
     * @param count The number of characters to erase. Defaults to the whole string.
     * @return The new value.
     */
    inline SafeString& erase(size_t index = 0, size_t count = std::string::npos) {
      check(); markAsUsed(); strPtr_->erase(index, count); return *this;
    }

    /**
     * Erase a specific character from the string.
     * @param position An iterator that points to the character to be erased.
     * @return An iterator that points to the character immediately following the erased character.
     */
    inline std::string::iterator erase(std::string::const_iterator position) {
      check(); markAsUsed(); return strPtr_->erase(position);
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
      check(); markAsUsed(); return strPtr_->erase(first, last);
    }

    /**
     * Append a character to the end of the string.
     * @param ch The character to append.
     */
    inline void push_back(char ch) { check(); markAsUsed(); strPtr_->push_back(ch); }

    /// Remove the last character from the string.
    inline void pop_back() { check(); markAsUsed(); strPtr_->pop_back(); }

    /**
     * Append a number of characters to the end of the string.
     * @param count The number of characters to append.
     * @param ch The character to append.
     * @return The new value.
     */
    inline SafeString& append(size_t count, char ch) {
      check(); markAsUsed(); strPtr_->append(count, ch); return *this;
    }

    /**
     * Append a SafeString to the end of the string.
     * @param str The string to append.
     * @return The new value.
     */
    inline SafeString& append(const SafeString& str) {
      check(); markAsUsed(); strPtr_->append(str.get()); return *this;
    }

    /**
     * Append a string to the end of the string.
     * @param str The string to append.
     * @return The new value.
     */
    inline SafeString& append(const std::string& str) {
      check(); markAsUsed(); strPtr_->append(str); return *this;
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
      check(); markAsUsed(); strPtr_->append(str.get(), pos, count); return *this;
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
      check(); markAsUsed(); strPtr_->append(str, pos, count); return *this;
    }

    /**
     * Append a C-style substring to the end of the string.
     * @param s A pointer to the character array to append.
     * @param count The number of characters to append.
     * @return The new value.
     */
    inline SafeString& append(const char* s, size_t count) {
      check(); markAsUsed(); strPtr_->append(s, count); return *this;
    }

    /**
     * Append a NULL-terminated C-style string to the end of the string.
     * @param s A pointer to the character array to append.
     * @return The new value.
     */
    inline SafeString& append(const char* s) {
      check(); markAsUsed(); strPtr_->append(s); return *this;
    }

    /**
     * Append a range of [first, last) characters to the end of the string.
     * @param first An iterator that points to the first character to append.
     * @param last An iterator that points to one past the last character to append.
     * @return The new value.
     */
    template <class InputIt>
    inline SafeString& append(InputIt first, InputIt last) {
      check(); markAsUsed(); strPtr_->append(first, last); return *this;
    }

    /**
     * Append characters from an initializer list to the end of a string.
     *  @param ilist The initializer list to append.
     * @return The new value.
     */
    inline SafeString& append(std::initializer_list<char> ilist) {
      check(); markAsUsed(); strPtr_->append(ilist); return *this;
    }

    /**
     * Compare the string to another SafeString.
     * @param str The string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(const SafeString& str) const { check(); return strPtr_->compare(str.get()); }

    /**
     * Compare the string to another string.
     * @param str The string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(const std::string& str) const { check(); return strPtr_->compare(str); }

    /**
     * Compare the string to another SafeString substring.
     * @param pos The index of the first character of the substring to compare to.
     * @param count The number of characters of the substring to compare to.
     * @param str The string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(size_t pos, size_t count, const SafeString& str) const {
      check(); return strPtr_->compare(pos, count, str.get());
    }

    /**
     * Compare the string to another substring.
     * @param pos The index of the first character of the substring to compare to.
     * @param count The number of characters of the substring to compare to.
     * @param str The string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(size_t pos, size_t count, const std::string& str) const {
      check(); return strPtr_->compare(pos, count, str);
    }

    /**
     * Compare a substring of this string to another SafeString substring
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
      size_t pos1, size_t count1, const SafeString& str,
      size_t pos2, size_t count2 = std::string::npos
    ) const {
      check(); return strPtr_->compare(pos1, count1, str.get(), pos2, count2);
    }

    /**
     * Compare a substring of this string to another substring.
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
      check(); return strPtr_->compare(pos1, count1, str, pos2, count2);
    }

    /**
     * Compare the string to another C-style string.
     * @param s The string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(const char* s) const { check(); return strPtr_->compare(s); }

    /**
     * Compare the string to another C-style substring.
     * @param pos The index of the first character of the substring to compare to.
     * @param count The number of characters of the substring to compare to.
     * @param s The string to compare to.
     * @return An integer less than, equal to, or greater than zero if the string
     * is less than, equal to, or greater than the compared string, respectively.
     */
    inline int compare(size_t pos, size_t count, const char* s) const {
      check(); return strPtr_->compare(pos, count, s);
    }

    /**constexpr int compare( size_type pos1, size_type count1,const CharT* s,
    size_type count2 ) const;
    @brief Compares the safe string to a substring of an array of characters.
    @return An integer less than, equal to, or greater than zero if the substring
    of the safe string is less than, equal to, or greater than the substring of
    the string s, respectively.
    */
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
      check(); return strPtr_->compare(pos1, count1, s, count2);
    }

    /**
     * Check if the string starts with a given substring.
     * @param sv The substring to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool starts_with(const std::string& sv) const { check(); return strPtr_->starts_with(sv); }

    /**
     * Check if the string starts with a given character.
     * @param ch The character to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool starts_with(char ch) const { check(); return strPtr_->starts_with(ch); }

    /**
     * Check if the string starts with a given C-style substring.
     * @param s The substring to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool starts_with(const char* s) const { check(); return strPtr_->starts_with(s); }

    /**
     * Check if the string ends with a given substring.
     * @param sv The substring to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool ends_with(const std::string& sv) const { check(); return strPtr_->ends_with(sv); }

    /**
     * Check if the string ends with a given character.
     * @param ch The character to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool ends_with(char ch) const { check(); return strPtr_->ends_with(ch); }

    /**
     * Check if the string ends with a given C-style substring.
     * @param s The substring to check for.
     * @return `true` if there's a match, `false` otherwise.
     */
    inline bool ends_with(const char* s) const { check(); return strPtr_->ends_with(s); }

    // TODO: contains (C++23)
    // constexpr bool contains( std::basic_string_view<CharT,Traits> sv ) const noexcept;

    /**
     * Replace part of this string with a SafeString.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters of this string to replace.
     * @param str The string to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(size_t pos, size_t count, const SafeString& str) {
      check(); markAsUsed(); strPtr_->replace(pos, count, str.get()); return *this;
    }

    /**
     * Replace part of this string with a string.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters of this string to replace.
     * @param str The string to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(size_t pos, size_t count, const std::string& str) {
      check(); markAsUsed(); strPtr_->replace(pos, count, str); return *this;
    }

    /**
     * Replace part of this string with a SafeString, using iterators.
     * @param first An iterator to the first character of this string to replace.
     * @param last An iterator to the last character of this string to replace.
     * @param str The string to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      std::string::const_iterator first, std::string::const_iterator last, const SafeString& str
    ) {
      check(); markAsUsed(); strPtr_->replace(first, last, str.get()); return *this;
    }

    /**
     * Replace part of this string with a string, using iterators.
     * @param first An iterator to the first character of this string to replace.
     * @param last An iterator to the last character of this string to replace.
     * @param str The string to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(
      std::string::const_iterator first, std::string::const_iterator last, const std::string& str
    ) {
      check(); markAsUsed(); strPtr_->replace(first, last, str); return *this;
    }

    /**
     * Replace part of this string with a SafeString substring.
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
      check(); markAsUsed(); strPtr_->replace(pos, count, str.get(), pos2, count2); return *this;
    }

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
      size_t pos, size_t count, const std::string& str, size_t pos2, size_t count2 = std::string::npos
    ) {
      check(); markAsUsed(); strPtr_->replace(pos, count, str, pos2, count2); return *this;
    }

    /**
     * Replace part of this string with a substring, using iterators.
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
      check(); markAsUsed(); strPtr_->replace(first, last, first2, last2); return *this;
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
      check(); strPtr_->replace(pos, count, cstr, count2); markAsUsed(); return *this;
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
      check(); markAsUsed(); strPtr_->replace(first, last, cstr, count); return *this;
    }

    /**
     * Replace part of this string with a C-style string.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters in this string to replace.
     * @param cstr The array of characters to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(size_t pos, size_t count, const char* cstr) {
      check(); markAsUsed(); strPtr_->replace(pos, count, cstr); return *this;
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
      check(); markAsUsed(); strPtr_->replace(first, last, cstr); return *this;
    }

    /**
     * Replace part of this string with a number of characters.
     * @param pos The index of the first character of this string to replace.
     * @param count The number of characters in this string to replace.
     * @param count2 The number of characters to replace.
     * @param ch The character to use as a replacement.
     * @return The new value.
     */
    inline SafeString& replace(size_t pos, size_t count, size_t count2, char ch) {
      check(); markAsUsed(); strPtr_->replace(pos, count, count2, ch); return *this;
    }

    /**
     * Replace part of this string with a number of characters, using iterators.
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
      check(); markAsUsed(); strPtr_->replace(first, last, count, ch); return *this;
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
      check(); markAsUsed(); strPtr_->replace(first, last, ilist); return *this;
    }

    /**
     * Get a substring of the string.
     * @param pos The index of the first character of the substring.
     * @param count The number of characters of the substring.
     * @return The substring itself.
     */
    inline SafeString substr(size_t pos = 0, size_t count = std::string::npos) const {
      check(); return SafeString(strPtr_->substr(pos, count));
    }

    /**
     * Copy a substring of the string to a buffer.
     * @param dest The buffer to copy to.
     * @param count The number of characters to copy.
     * @param pos The index of the first character to copy.a
     * @return The number of characters that were copied.
     */
    inline size_t copy(char* dest, size_t count, size_t pos = 0) const {
      check(); return strPtr_->copy(dest, count, pos);
    }

    /**
     * Resize the string.
     * @param count The new size of the string.
     */
    inline void resize(size_t count) { check(); markAsUsed(); strPtr_->resize(count); }

    /**
     * Resize the safe string and fill the extra space with a given character.
     * @param count The new size of the string.
     * @param ch The character to use as filling.
     */
    inline void resize(size_t count, char ch) { check(); markAsUsed(); strPtr_->resize(count, ch); }

    /**
     * Swap the contents of this string with another SafeString.
     * @param other The string to swap with.
     */
    inline void swap(SafeString& other) {
      check(); other.check(); markAsUsed(); other.markAsUsed(); strPtr_.swap(other.strPtr_);
    }

    /**
     * Find the first occurrence of a given SafeString.
     * @param str The string to find.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find(const SafeString& str, size_t pos = 0) const {
      check(); return strPtr_->find(str.get(), pos);
    }

    /**
     * Find the first occurrence of a given string.
     * @param str The string to find.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find(const std::string& str, size_t pos = 0) const {
      check(); return strPtr_->find(str, pos);
    }

    /**
     * Find the first occurrence of a given C-style substring.
     * @param s The string to find.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find(const char* s, size_t pos, size_t count) const {
      check(); return strPtr_->find(s, pos, count);
    }

    /**
     * Find the first occurrence of a given C-style string.
     * @param s The string to find.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find(const char* s, size_t pos = 0) const {
      check(); return strPtr_->find(s, pos);
    }

    /**
     * Find the first occurrence of a given character.
     * @param ch The character to find.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find(char ch, size_t pos = 0) const {
      check(); return strPtr_->find(ch, pos);
    }

    /**
     * Find the last occurrence of a given SafeString.
     * @param str The string to find.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t rfind(const SafeString& str, size_t pos = std::string::npos) const {
      check(); return strPtr_->rfind(str.get(), pos);
    }

    /**
     * Find the last occurrence of a given string.
     * @param str The string to find.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t rfind(const std::string& str, size_t pos = std::string::npos) const {
      check(); return strPtr_->rfind(str, pos);
    }

    /**
     * Find the last occurrence of a given C-style substring.
     * @param s The string to find.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t rfind(const char* s, size_t pos, size_t count) const {
      check(); return strPtr_->rfind(s, pos, count);
    }

    /**
     * Find the last occurrence of a given C-style string.
     * @param s The string to find.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t rfind(const char* s, size_t pos = std::string::npos) const {
      check(); return strPtr_->rfind(s, pos);
    }

    /**
     * Find the last occurrence of a given character.
     * @param ch The character to find.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t rfind(char ch, size_t pos = std::string::npos) const {
      check(); return strPtr_->rfind(ch, pos);
    }

    /**
     * Find the first occurrence of any of the characters in a given SafeString.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_of(const SafeString& str, size_t pos = 0) const {
      check(); return strPtr_->find_first_of(str.get(), pos);
    }

    /**
     * Find the first occurrence of any of the characters in a given string.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_of(const std::string& str, size_t pos = 0) const {
      check(); return strPtr_->find_first_of(str, pos);
    }

    /**
     * Find the first occurrence of any of the characters in a given C-style substring.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_of(const char* s, size_t pos, size_t count) const {
      check(); return strPtr_->find_first_of(s, pos, count);
    }

    /**
     * Find the first occurrence of any of the characters in a given C-style string.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_of(const char* s, size_t pos = 0) const {
      check(); return strPtr_->find_first_of(s, pos);
    }

    /**
     * Find the first occurrence of a given character.
     * @param ch The character to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_of(char ch, size_t pos = 0) const {
      check(); return strPtr_->find_first_of(ch, pos);
    }

    /**
     * Find the first occurrence of none of the characters in a given SafeString.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_not_of(const SafeString& str, size_t pos = 0) const {
      check(); return strPtr_->find_first_not_of(str.get(), pos);
    }

    /**
     * Find the first occurrence of none of the characters in a given string.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_not_of(const std::string& str, size_t pos = 0) const {
      check(); return strPtr_->find_first_not_of(str, pos);
    }

    /**
     * Find the first occurrence of none of the characters in a given C-style substring.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_not_of(const char* s, size_t pos, size_t count) const {
      check(); return strPtr_->find_first_not_of(s, pos, count);
    }

    /**
     * Find the first occurrence of none of the characters in a given C-style string.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_not_of(const char* s, size_t pos = 0) const {
      check(); return strPtr_->find_first_not_of(s, pos);
    }

    /**
     * Find the first occurrence that's not the given character.
     * @param ch The character to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the start of the string.
     * @return The index of the first occurrence, or std::string::npos if not found.
     */
    inline size_t find_first_not_of(char ch, size_t pos = 0) const {
      check(); return strPtr_->find_first_not_of(ch, pos);
    }

    /**
     * Find the last occurrence of any of the characters in a given SafeString.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_of(const SafeString& str, size_t pos = std::string::npos) const {
      check(); return strPtr_->find_last_of(str.get(), pos);
    }

    /**
     * Find the last occurrence of any of the characters in a given string.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_of(const std::string& str, size_t pos = std::string::npos) const {
      check(); return strPtr_->find_last_of(str, pos);
    }

    /**
     * Find the last occurrence of any of the characters in a given C-style substring.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_of(const char* s, size_t pos, size_t count) const {
      check(); return strPtr_->find_last_of(s, pos, count);
    }

    /**
     * Find the last occurrence of any of the characters in a given C-style string.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_of(const char* s, size_t pos = std::string::npos) const {
      check(); return strPtr_->find_last_of(s, pos);
    }

    /**
     * Find the last occurrence of a given character.
     * @param ch The character to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_of(char ch, size_t pos = std::string::npos) const {
      check(); return strPtr_->find_last_of(ch, pos);
    }

    /**
     * Find the last occurrence of none of the characters in a given SafeString.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_not_of(const SafeString& str, size_t pos = std::string::npos) const {
      check(); return strPtr_->find_last_not_of(str.get(), pos);
    }

    /**
     * Find the last occurrence of none of the characters in a given string.
     * @param str The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_not_of(const std::string& str, size_t pos = std::string::npos) const {
      check(); return strPtr_->find_last_not_of(str, pos);
    }

    /**
     * Find the last occurrence of none of the characters in a given C-style substring.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search.
     * @param count The number of characters to search.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_not_of(const char* s, size_t pos, size_t count) const {
      check(); return strPtr_->find_last_not_of(s, pos, count);
    }

    /**
     * Find the last occurrence of none of the characters in a given C-style string.
     * @param s The string to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_not_of(const char* s, size_t pos = std::string::npos) const {
      check(); return strPtr_->find_last_not_of(s, pos);
    }

    /**
     * Find the last occurrence that's not the given character.
     * @param ch The character to use as a reference for searching.
     * @param pos The index of the first character to search. Defaults to the end of the string.
     * @return The index of the last occurrence, or std::string::npos if not found.
     */
    inline size_t find_last_not_of(char ch, size_t pos = std::string::npos) const {
      check(); return strPtr_->find_last_not_of(ch, pos);
    }

    /// Assignment operator.
    inline SafeString& operator=(const SafeString& other) {
      check(); markAsUsed(); *strPtr_ = other.get(); return *this;
    }

    /// Assignment operator.
    inline SafeString& operator=(const std::string& other) {
      check(); markAsUsed(); *strPtr_ = other; return *this;
    }

    /// Assignment operator.
    inline SafeString& operator=(const char* s) {
      check(); markAsUsed(); *strPtr_ = s; return *this;
    }

    /// Assignment operator.
    inline SafeString& operator=(char ch) {
      check(); markAsUsed(); *strPtr_ = ch; return *this;
    }

    /// Assignment operator.
    inline SafeString& operator=(std::initializer_list<char> ilist) {
      check(); markAsUsed(); *strPtr_ = ilist; return *this;
    }

    /// Compound assignment operator.
    inline SafeString& operator+=(const SafeString& str) {
      check(); markAsUsed(); strPtr_->operator+=(str.get()); return *this;
    }

    /// Compound assignment operator.
    inline SafeString& operator+=(const std::string& str) {
      check(); markAsUsed(); strPtr_->operator+=(str); return *this;
    }

    /// Compound assignment operator.
    inline SafeString& operator+=(char ch) {
      check(); markAsUsed(); strPtr_->operator+=(ch); return *this;
    }

    /// Compound assignment operator.
    inline SafeString& operator+=(const char* s) {
      check(); markAsUsed(); strPtr_->operator+=(s); return *this;
    }

    /// Compound assignment operator.
    inline SafeString& operator+=(std::initializer_list<char> ilist) {
      check(); markAsUsed(); strPtr_->operator+=(ilist); return *this;
    }

    /// Subscript/Indexing operator.
    inline char& operator[](size_t pos) {
      check(); markAsUsed(); return strPtr_->operator[](pos);
    }

    /// Subscript/Indexing operator.
    inline const char& operator[](size_t pos) const {
      check(); return strPtr_->operator[](pos);
    }

    /// Concat operator.
    inline SafeString operator+(const SafeString& rhs) const {
      check(); return SafeString(*strPtr_ + rhs.get());
    };

    /// Concat operator.
    inline SafeString operator+(const std::string rhs) const {
      check(); return SafeString(*strPtr_ + rhs);
    };

    /// Concat operator.
    inline SafeString operator+(const char* rhs) const {
      check(); return SafeString(*strPtr_ + rhs);
    };

    /// Concat operator.
    inline SafeString operator+(char rhs) const {
      check(); return SafeString(*strPtr_ + rhs);
    };

    /// Equality operator.
    inline bool operator==(const SafeString& rhs) const {
      check(); return *strPtr_ == rhs.get();
    };

    /// Equality operator.
    inline bool operator==(const std::string& rhs) const {
      check(); return *strPtr_ == rhs;
    };

    /// Equality operator.
    inline bool operator==(const char* rhs) const {
      check(); return *strPtr_ == rhs;
    };

    /// Inequality operator.
    inline bool operator!=(const SafeString& rhs) const {
      check(); return *strPtr_ != rhs.get();
    };

    /// Inequality operator.
    inline bool operator!=(const std::string& rhs) const {
      check(); return *strPtr_ != rhs;
    };

    /// Inequality operator.
    inline bool operator!=(const char* rhs) const {
      check(); return *strPtr_ != rhs;
    };

    /// Lesser comparison operator.
    inline bool operator<(const SafeString& rhs) const {
      check(); return *strPtr_ < rhs.get();
    };

    /// Lesser comparison operator.
    inline bool operator<(const std::string& rhs) const {
      check(); return *strPtr_ < rhs;
    };

    /// Lesser comparison operator.
    inline bool operator<(const char* rhs) const {
      check(); return *strPtr_ < rhs;
    };

    /// Greater comparison operator.
    inline bool operator>(const SafeString& rhs) const {
      check(); return *strPtr_ > rhs.get();
    };

    /// Greater comparison operator.
    inline bool operator>(const std::string& rhs) const {
      check(); return *strPtr_ > rhs;
    };

    /// Greater comparison operator.
    inline bool operator>(const char* rhs) const {
      check(); return *strPtr_ > rhs;
    };

    /// Lesser-or-equal comparison operator.
    inline bool operator<=(const SafeString& rhs) const {
      check(); return *strPtr_ <= rhs.get();
    };

    /// Lesser-or-equal comparison operator.
    inline bool operator<=(const std::string& rhs) const {
      check(); return *strPtr_ <= rhs;
    };

    /// Lesser-or-equal comparison operator.
    inline bool operator<=(const char* rhs) const {
      check(); return *strPtr_ <= rhs;
    };

    /// Greater-or-equal comparison operator.
    inline bool operator>=(const SafeString& rhs) const {
      check(); return *strPtr_ >= rhs.get();
    };

    /// Greater-or-equal comparison operator.
    inline bool operator>=(const std::string& rhs) const {
      check(); return *strPtr_ >= rhs;
    };

    /// Greater-or-equal comparison operator.
    inline bool operator>=(const char* rhs) const {
      check(); return *strPtr_ >= rhs;
    };
};

/**
 * Overload of the bitwise <<operator for SafeStrings.
 * @param _out The output stream to write to.
 * @param _t The safe string to write.
 * @return The output stream.
*/
inline std::ostream& operator<<(std::ostream& _out, SafeString const& _t) {
  _out << _t.get(); return _out;
}

#endif  // SAFESTRING_H
