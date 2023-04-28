#ifndef SAFESTRING_H
#define SAFESTRING_H

#include "safebase.h"
#include <memory>
#include <string>

/**
 * Safe wrapper for a string variable.
 * This class is used to safely store a string variable within a contract.
 * @see SafeBase
 */
class SafeString : SafeBase {
private:
  std::string str;                             ///< string value.
  mutable std::unique_ptr<std::string> strPtr; ///< Pointer to the string value.

  /**
   * Check if the strPtr is initialized.
   * If not, initialize it.
   */
  void check() const override {
    if (strPtr == nullptr) {
      strPtr = std::make_unique<std::string>(str);
    }
  }

public:
  /** Constructor.
   * Only Variables built with this constructor will be registered within
   * a contract.
   * @param owner The contract that owns this variable.
   * @param str The string initial value (default: "").
   */
  SafeString(DynamicContract *owner, const std::string &str = std::string())
      : SafeBase(owner), strPtr(std::make_unique<std::string>(str)) {}

  /** Default constructor
   */
  SafeString() : SafeBase(nullptr), strPtr(std::make_unique<std::string>()) {}

  /** Constructor from string
   * @param str The string initial value.
   */
  explicit SafeString(const std::string &str)
      : SafeBase(nullptr), strPtr(std::make_unique<std::string>(str)) {}

  /** Copy constructor
   * @param other The SafeString to copy.
   */
  SafeString(const SafeString &other) : SafeBase(nullptr) {
    other.check();
    strPtr = std::make_unique<std::string>(*other.strPtr);
  }

  /** constexpr basic_string& assign( size_type count, CharT ch );
  @brief Assigns a new value to the safe string, replacing its current contents.
  @param count The number of characters to assign.
  @param ch The character to fill the string with.
  @return *this
  */
  inline SafeString &assign(size_t count, char ch) {
    check();
    markAsUsed();
    strPtr->assign(count, ch);
    return *this;
  }

  /** constexpr basic_string& assign( const basic_string& str );2
  @brief Assigns a new value to the safe string, using another safe string.
  @param str Another safe string object, whose value is either copied or moved.
  @return *this
  */
  inline SafeString &assign(const SafeString &str) {
    check();
    markAsUsed();
    strPtr->assign(str.get());
    return *this;
  }

  /**constexpr basic_string& assign( const basic_string& str, size_type pos,
  size_type count = npos);
  @brief Assigns a new value to the safe string, using a safe substring.
  @param str Another safe string object, whose value is either copied or moved.
  @param pos Position of the first character in str that is copied to the
  object as a substring.
  @param count Number of characters to include in the substring (if the string
  is shorter, as many characters as possible are used).
  @return *this
  */
  inline SafeString &assign(const SafeString &str, size_t pos,
                            size_t count = std::string::npos) {
    check();
    markAsUsed();
    strPtr->assign(str.get(), pos, count);
    return *this;
  }

  /** constexpr basic_string& assign( const CharT* s, size_type count );
  @brief Assigns a new value to the string, replacing its current contents,
  using a character array.
  @param s A pointer to an array of characters.
  @param count The number of characters to copy.
  @return *this
  */
  inline SafeString &assign(const char *s, size_t count) {
    check();
    markAsUsed();
    strPtr->assign(s, count);
    return *this;
  }

  /** constexpr basic_string& assign( const CharT* s );
  @brief Assigns a new value to the string, replacing its current contents,
  using a null-terminated character array.
  @param s A pointer to an array of characters.
  @return *this
  */
  inline SafeString &assign(const char *s) {
    check();
    markAsUsed();
    strPtr->assign(s);
    return *this;
  }

  /** constexpr basic_string& assign( InputIt first, InputIt last );
  @brief Assigns a new value to the string, replacing its current contents,
  using iterators.
  @param first An input iterator.
  @param last An input iterator.
  @return *this
  */
  template <class InputIt>
  inline SafeString &assign(InputIt first, InputIt last) {
    check();
    markAsUsed();
    strPtr->assign(first, last);
    return *this;
  }

  /** constexpr basic_string& assign( std::initializer_list<CharT> ilist );
  @brief Assigns a new value to the string, replacing its current contents,
  using a initializer list.
  @param ilist An initializer_list object.
  @return *this
  */
  inline SafeString &assign(std::initializer_list<char> ilist) {
    check();
    markAsUsed();
    strPtr->assign(ilist);
    return *this;
  }

  /** constexpr reference at( size_type pos );
  @brief Returns a reference to the character at specified location pos.
  @param pos Position of the character to return.
  @return A reference to the character at specified location pos.
  */
  inline char &at(size_t pos) {
    check();
    markAsUsed();
    return strPtr->at(pos);
  }

  /** constexpr const_reference at( size_type pos ) const;
  @brief Returns a reference to the character at specified location pos.
  @param pos Position of the character to return.
  @return A reference to the character at specified location pos.
  */
  inline const char &at(size_t pos) const {
    check();
    return strPtr->at(pos);
  }

  /** constexpr reference front();
  @brief Returns a reference to the first character in the string.
  @return A reference to the first character in the string.
  */
  inline char &front() {
    check();
    markAsUsed();
    return strPtr->front();
  }

  /** constexpr const_reference front() const;
  @brief Returns a reference to the first character in the string.
  @return A reference to the first character in the string.
  */
  inline const char &front() const {
    check();
    return strPtr->front();
  }

  /** constexpr reference back();
  @brief Returns a reference to the last character in the string.
  @return A reference to the last character in the string.
  */
  inline char &back() {
    check();
    markAsUsed();
    return strPtr->back();
  }

  /** constexpr const_reference back() const;
  @brief Returns a reference to the last character in the string.
  @return A reference to the last character in the string.
  */
  inline const char &back() const {
    check();
    return strPtr->back();
  }

  /** constexpr const CharT* c_str() const noexcept;
  @brief Returns a pointer to an array that contains a null-terminated sequence
  of characters (i.e., a C-string) representing the current value of the string
  object.
  @return A pointer to the null-terminated character sequence.
  */
  inline const char *c_str() const {
    check();
    return strPtr->c_str();
  }

  /** constexpr const CharT* data() const noexcept;
  @brief Returns a pointer to an array that contains the same sequence of
  characters as the characters that make up the value of the string object.
  @return A pointer to the first character of the string.
  */
  inline const char *data() const {
    check();
    return strPtr->data();
  }

  /**
  @brief Returns an iterator to the beginning of the string.
  @return An iterator to the beginning of the string.
  */
  inline std::string::iterator begin() {
    check();
    markAsUsed();
    return strPtr->begin();
  }

  /**
  @brief Returns a const iterator to the beginning of the string.
  @return A const iterator to the beginning of the string.
  */
  inline std::string::const_iterator cbegin() const {
    check();
    return strPtr->cbegin();
  }

  /**
  @brief Returns an iterator to the end of the string.
  @return An iterator to the end of the string.
  */
  inline std::string::iterator end() {
    check();
    markAsUsed();
    return strPtr->end();
  }

  /**
  @brief Returns a const iterator to the end of the string.
  @return A const iterator to the end of the string.
  */
  inline std::string::const_iterator cend() const {
    check();
    return strPtr->cend();
  }

  /**
  @brief Returns a reverse iterator to the beginning of the reversed string.
  @return A reverse iterator to the beginning of the reversed string.
  */
  inline std::string::reverse_iterator rbegin() {
    check();
    markAsUsed();
    return strPtr->rbegin();
  }

  /**
  @brief Returns a const reverse iterator to the beginning of the reversed
  string.
  @return A const reverse iterator to the beginning of the reversed string.
  */
  inline std::string::const_reverse_iterator crbegin() {
    check();
    return strPtr->crbegin();
  }

  /**
  @brief Returns a reverse iterator to the end of the reversed string.
  @return A reverse iterator to the end of the reversed string.
  */
  inline std::string::reverse_iterator rend() {
    check();
    markAsUsed();
    return strPtr->rend();
  }

  /**
  @brief Returns a const reverse iterator to the end of the reversed string.
  @return A const reverse iterator to the end of the reversed string.
  */
  inline std::string::const_reverse_iterator crend() {
    check();
    return strPtr->crend();
  }

  /**
  @brief Checks if the string has no characters.
  @return True if the string is empty, false otherwise.
  */
  inline bool empty() const {
    check();
    return strPtr->empty();
  }

  /**
  @brief Get the number of characters in the string.
  @return The number of characters in the string.
  */
  inline size_t size() const {
    check();
    return strPtr->size();
  }

  /**
  @brief Get the number of characters in the string.
  @return The number of characters in the string.
  */
  inline size_t length() const {
    check();
    return strPtr->length();
  }

  /**
  @brief Get the maximum number of characters the string can hold.
  @return The maximum number of characters the string can hold.
  */
  inline size_t max_size() const {
    check();
    return strPtr->max_size();
  }

  /**
  @brief Increase the capacity of the string to a value that's greater or equal
  to new_cap.
  @param new_cap The new capacity of the string.
  */
  inline void reserve(size_t new_cap) {
    check();
    markAsUsed();
    strPtr->reserve(new_cap);
  }

  /**
  @brief Get the number of characters that can be held in currently allocated
  storage.
  @return The number of characters that can be held in currently allocated
  storage.
  */
  inline size_t capacity() const {
    check();
    return strPtr->capacity();
  }

  /**
  @brief Request the removal of unused capacity.
  */
  inline void shrink_to_fit() {
    check();
    markAsUsed();
    strPtr->shrink_to_fit();
  }

  /** constexpr void clear() noexcept;
  @brief Clear the contents of the string.
  */
  inline void clear() {
    check();
    markAsUsed();
    strPtr->clear();
  }
  /** constexpr basic_string& insert( size_type index, size_type count, CharT ch
  );
  @brief Inserts count copies of character ch at the position index.
  @param index The index at which the characters will be inserted.
  @param count The number of characters to insert.
  @param ch The character to insert.
  @return A reference to the safe string.
  */
  inline SafeString &insert(size_t index, size_t count, char ch) {
    check();
    markAsUsed();
    strPtr->insert(index, count, ch);
    return *this;
  }
  /** constexpr basic_string& insert( size_type index, const CharT* s );
  @brief Inserts a null-terminated character string pointed to by s at the
  position index.
  @param index The index at which the characters will be inserted.
  @param s The string to insert.
  @return A reference to the safe string.
  */
  inline SafeString &insert(size_t index, const char *s) {
    check();
    markAsUsed();
    strPtr->insert(index, s);
    return *this;
  }

  /** constexpr basic_string& insert( size_type index, const CharT* s, size_type
  count );
  @brief Inserts the first count characters of the character string pointed to
  by s at the position index.
  @param index The index at which the characters will be inserted.
  @param s The string to insert.
  @param count The number of characters to insert.
  @return A reference to the safe string.
  */
  inline SafeString &insert(size_t index, const char *s, size_t count) {
    check();
    markAsUsed();
    strPtr->insert(index, s, count);
    return *this;
  }

  /** constexpr basic_string& insert( size_type index, const basic_string& str
  );
  @brief Inserts a copy of str (safe) at the position index.
  @param index The index at which the characters will be inserted.
  @param str The safe string to insert.
  @return A reference to the safe string.
  */
  inline SafeString &insert(size_t index, const SafeString &str) {
    check();
    markAsUsed();
    strPtr->insert(index, str.get());
    return *this;
  }

  /** constexpr basic_string& insert( size_type index, const basic_string& str
  );
  @brief Inserts a copy of str at the position index.
  @param index The index at which the characters will be inserted.
  @param str The string to insert.
  @return A reference to the safe string.
  */
  inline SafeString &insert(size_t index, const std::string &str) {
    check();
    markAsUsed();
    strPtr->insert(index, str);
    return *this;
  }

  /** constexpr basic_string& insert( size_type index, const basic_string& str,
  size_type index_str, size_type count = npos );
  @brief Inserts a copy of a substring of str (safe). The substring consists of
  the count characters of str starting at position index_str.
  @param index The index at which the characters will be inserted.
  @param str The safe string to insert.
  @param index_str The index of the first character of str to insert.
  @param count The number of characters to insert.
  @return A reference to the safe string.
  */
  inline SafeString &insert(size_t index, const SafeString &str,
                            size_t index_str,
                            size_t count = std::string::npos) {
    check();
    markAsUsed();
    strPtr->insert(index, str.get(), index_str, count);
    return *this;
  }

  /** constexpr basic_string& insert( size_type index, const basic_string& str,
  size_type index_str, size_type count = npos );
  @brief Inserts a copy of a substring of str. The substring consists of the
  count characters of str starting at position index_str.
  @param index The index at which the characters will be inserted.
  @param str The string to insert.
  @param index_str The index of the first character of str to insert.
  @param count The number of characters to insert.
  @return A reference to the safe string.
  */
  inline SafeString &insert(size_t index, const std::string &str,
                            size_t index_str,
                            size_t count = std::string::npos) {
    check();
    markAsUsed();
    strPtr->insert(index, str, index_str, count);
    return *this;
  }

  /** constexpr iterator insert( const_iterator pos, CharT ch );
  @brief Inserts character ch before the character pointed to by pos.
  @param pos The position at which the character will be inserted.
  @param ch The character to insert.
  @return An iterator that points to the inserted character.
  */
  inline std::string::iterator insert(std::string::const_iterator pos,
                                      char ch) {
    check();
    markAsUsed();
    return strPtr->insert(pos, ch);
  }

  /** constexpr iterator insert( const_iterator pos, size_type count, CharT ch
  );
  @brief Inserts count copies of character ch before the character pointed to by
  pos.
  @param pos The position at which the characters will be inserted.
  @param count The number of characters to insert.
  @param ch The character to insert.
  @return An iterator that points to the first inserted character.
  */
  inline std::string::iterator insert(std::string::const_iterator pos,
                                      size_t count, char ch) {
    check();
    markAsUsed();
    return strPtr->insert(pos, count, ch);
  }

  /** constexpr iterator insert( const_iterator pos, InputIt first, InputIt last
  );
  @brief Inserts characters in the range [first, last) before the character
  pointed to by pos.
  @param pos The position at which the characters will be inserted.
  @param first An iterator that points to the first character to insert.
  @param last An iterator that points to one past the last character to insert.
  @return An iterator that points to the first inserted character.
  */
  template <class InputIt>
  inline std::string::iterator insert(std::string::const_iterator pos,
                                      InputIt first, InputIt last) {
    check();
    markAsUsed();
    return strPtr->insert(pos, first, last);
  }

  /** constexpr iterator insert( const_iterator pos,
  std::initializer_list<CharT> ilist );
  @brief Inserts characters from initializer list ilist before the character
  pointed to by pos.
  @param pos The position at which the characters will be inserted.
  @param ilist The initializer list to insert.
  @return An iterator that points to the first inserted character.
  */
  inline std::string::iterator insert(std::string::const_iterator pos,
                                      std::initializer_list<char> ilist) {
    check();
    markAsUsed();
    return strPtr->insert(pos, ilist);
  }

  /** constexpr basic_string& erase( size_type index = 0, size_type count = npos
  );
  @brief Erases count characters starting at index.
  @param index The index of the first character to erase.
  @param count The number of characters to erase.
  @return A reference to the safe string.
  */
  inline SafeString &erase(size_t index = 0, size_t count = std::string::npos) {
    check();
    markAsUsed();
    strPtr->erase(index, count);
    return *this;
  }

  /** constexpr iterator erase( const_iterator position );
  @brief Erases the character pointed to by position.
  @param position An iterator that points to the character to erase.
  @return An iterator that points to the character immediately following the
  erased character.
  */
  inline std::string::iterator erase(std::string::const_iterator position) {
    check();
    markAsUsed();
    return strPtr->erase(position);
  }

  /** constexpr iterator erase( const_iterator first, const_iterator last );
  @brief Erases the characters in the range [first, last).
  @param first An iterator that points to the first character to erase.
  @param last An iterator that points to one past the last character to erase.
  @return An iterator that points to the character pointed to by last prior to
  erasing, or end() if no such character exists.
  */
  inline std::string::iterator erase(std::string::const_iterator first,
                                     std::string::const_iterator last) {
    check();
    markAsUsed();
    return strPtr->erase(first, last);
  }

  /** constexpr void push_back( CharT ch );
  @brief Appends character ch to the end of the string.
  @param ch The character to append.
  */
  inline void push_back(char ch) {
    check();
    markAsUsed();
    strPtr->push_back(ch);
  }

  /** constexpr void pop_back();
  @brief Removes the last character from the string.
  */
  inline void pop_back() {
    check();
    markAsUsed();
    strPtr->pop_back();
  }

  /** constexpr basic_string& append( size_type count, CharT ch );
  @brief Appends count copies of character ch to the safe string.
  @param count The number of characters to append.
  @param ch The character to append.
  @return A reference to the safe string.
  */
  inline SafeString &append(size_t count, char ch) {
    check();
    markAsUsed();
    strPtr->append(count, ch);
    return *this;
  }

  /** constexpr basic_string& append( const basic_string& str );
  @brief Appends a copy of str (safe) to the safe string.
  @param str The safe string to append.
  @return A reference to the safe string.
  */
  inline SafeString &append(const SafeString &str) {
    check();
    markAsUsed();
    strPtr->append(str.get());
    return *this;
  }

  /** constexpr basic_string& append( const basic_string& str );
  @brief Appends a copy of str to the safe string.
  @param str The string to append.
  @return A reference to the safe string.
  */
  inline SafeString &append(const std::string &str) {
    check();
    markAsUsed();
    strPtr->append(str);
    return *this;
  }

  /** constexpr basic_string& append( const basic_string& str, size_type pos,
  size_type count = npos );
  @brief Appends a copy of the safe substring in str, starting at position pos
  and continuing for count characters (or until the end of str, if either str is
  too short or if count is npos).
  @param str The safe string to append.
  @param pos The index of the first character to append.
  @param count The number of characters to append.
  @return A reference to the safe string.
  */
  inline SafeString &append(const SafeString &str, size_t pos,
                            size_t count = std::string::npos) {
    check();
    markAsUsed();
    strPtr->append(str.get(), pos, count);
    return *this;
  }

  /** constexpr basic_string& append( const basic_string& str, size_type pos,
  size_type count = npos );
  @brief Appends a copy of the substring in str, starting at position pos and
  continuing for count characters (or until the end of str, if either str is too
  short or if count is npos).
  @param str The string to append.
  @param pos The index of the first character to append.
  @param count The number of characters to append.
  @return A reference to the safe string.
  */
  inline SafeString &append(const std::string &str, size_t pos,
                            size_t count = std::string::npos) {
    check();
    markAsUsed();
    strPtr->append(str, pos, count);
    return *this;
  }

  /** constexpr basic_string& append( const CharT* s, size_type count );
  @brief Appends a copy of the first count characters in the array of characters
  pointed to by s.
  @param s A pointer to the character array to append.
  @param count The number of characters to append.
  @return A reference to the safe string.
  */
  inline SafeString &append(const char *s, size_t count) {
    check();
    markAsUsed();
    strPtr->append(s, count);
    return *this;
  }

  /** constexpr basic_string& append( const CharT* s );
  @brief Appends a copy of the null-terminated character array pointed to by s.
  @param s A pointer to the character array to append.
  @return A reference to the safe string.
  */
  inline SafeString &append(const char *s) {
    check();
    markAsUsed();
    strPtr->append(s);
    return *this;
  }

  /** constexpr basic_string& append( InputIt first, InputIt last );
  @brief Appends a copy of the range [first, last) to the safe string.
  @param first An iterator that points to the first character to append.
  @param last An iterator that points to one past the last character to append.
  @return A reference to the string.
  */
  template <class InputIt>
  inline SafeString &append(InputIt first, InputIt last) {
    check();
    markAsUsed();
    strPtr->append(first, last);
    return *this;
  }

  /** constexpr basic_string& append( std::initializer_list<CharT> ilist );
  @brief Appends a copy of the characters in the initializer list ilist to the
  safe string.
  @param ilist The initializer list to append.
  @return A reference to the safe string.
  */
  inline SafeString &append(std::initializer_list<char> ilist) {
    check();
    markAsUsed();
    strPtr->append(ilist);
    return *this;
  }

  /** constexpr int compare( const basic_string& str ) const noexcept;
  @brief Compares the safe string to another safe string.
  @param str The safe string to compare to.
  @return An integer less than, equal to, or greater than zero if the safe
  string is less than, equal to, or greater than the safe string str,
  respectively.
  */
  inline int compare(const SafeString &str) const {
    check();
    return strPtr->compare(str.get());
  }
  /** constexpr int compare( const basic_string& str ) const noexcept;
  @brief Compares the safe string to str.
  @param str The string to compare to.
  @return An integer less than, equal to, or greater than zero if the safe
  string is less than, equal to, or greater than str, respectively.
  */
  inline int compare(const std::string &str) const {
    check();
    return strPtr->compare(str);
  }

  /** constexpr int compare( size_type pos1, size_type count1, const
  basic_string& str ) const;
  @brief Compares the safe string to a substring of safe string str.
  @param pos1 The index of the first character of the substring of the safe
  string to compare.
  @param count1 The number of characters in the substring of the safe string to
  compare.
  @param str The safe string to compare to.
  @return An integer less than, equal to, or greater than zero if the substring
  of the safe string is less than, equal to, or greater than the safe string
  str, respectively.
  */
  inline int compare(size_t pos1, size_t count1, const SafeString &str) const {
    check();
    return strPtr->compare(pos1, count1, str.get());
  }

  /** constexpr int compare( size_type pos1, size_type count1, const
  basic_string& str ) const;
  @brief Compares the safe string to a substring of str.
  @param pos1 The index of the first character of the substring of the safe
  string to compare.
  @param count1 The number of characters in the substring of the safe string to
  compare.
  @param str The string to compare to.
  @return An integer less than, equal to, or greater than zero if the substring
  of the safe string is less than, equal to, or greater than str, respectively.
  */
  inline int compare(size_t pos1, size_t count1, const std::string &str) const {
    check();
    return strPtr->compare(pos1, count1, str);
  }

  /** constexpr int compare( size_type pos1, size_type count1, const
  basic_string& str, size_type pos2, size_type count2 = npos ) const
  @brief Compares the safe string to a substring of safe string str.
  @param pos1 The index of the first character of the substring of the safe
  string to compare.
  @param count1 The number of characters in the substring of the safe string to
  compare.
  @param str The safe string to compare to.
  @param pos2 The index of the first character of the substring of the safe
  string str to compare.
  @param count2 The number of characters in the substring of the safe string str
  to compare.
  @return An integer less than, equal to, or greater than zero if the substring
  of the safe string is less than, equal to, or greater than the substring of
  the safe string str, respectively.
  */
  inline int compare(size_t pos1, size_t count1, const SafeString &str,
                     size_t pos2, size_t count2 = std::string::npos) const {
    check();
    return strPtr->compare(pos1, count1, str.get(), pos2, count2);
  }

  /** constexpr int compare( size_type pos1, size_type count1, const
  basic_string& str, size_type pos2, size_type count2 = npos ) const
  @brief Compares the safe string to a substring of str.
  @param pos1 The index of the first character of the substring of the safe
  string to compare.
  @param count1 The number of characters in the substring of the safe string to
  compare.
  @param str The string to compare to.
  @param pos2 The index of the first character of the substring of the safe
  string str to compare.
  @param count2 The number of characters in the substring of the safe string str
  to compare.
  @return An integer less than, equal to, or greater than zero if the substring
  of the safe string is less than, equal to, or greater than the substring of
  str, respectively.
  */
  inline int compare(size_t pos1, size_t count1, const std::string &str,
                     size_t pos2, size_t count2 = std::string::npos) const {
    check();
    return strPtr->compare(pos1, count1, str, pos2, count2);
  }

  /** constexpr int compare( const CharT* s ) const;
  @brief Compares the safe string to a array of characters.
  @param s The string to compare to.
  @return An integer less than, equal to, or greater than zero if the safe
  string is less than, equal to, or greater than the string s, respectively.
  */
  inline int compare(const char *s) const {
    check();
    return strPtr->compare(s);
  }

  /** constexpr int compare( size_type pos1, size_type count1, const CharT* s )
  const;
  @brief Compares the safe string to a substring of an array of characters.
  @param pos1 The index of the first character of the substring of the safe
  string to compare.
  @param count1 The number of characters in the substring of the safe string to
  compare.
  @param s The string to compare to.
  @return An integer less than, equal to, or greater than zero if the substring
  of the safe string is less than, equal to, or greater than the string s,
  respectively.
  */
  inline int compare(size_t pos1, size_t count1, const char *s) const {
    check();
    return strPtr->compare(pos1, count1, s);
  }

  /**constexpr int compare( size_type pos1, size_type count1,const CharT* s,
  size_type count2 ) const;
  @brief Compares the safe string to a substring of an array of characters.
  @param pos1 The index of the first character of the substring of the safe
  string to compare.
  @param count1 The number of characters in the substring of the safe string to
  compare.
  @param s The string to compare to.
  @param count2 The number of characters in the substring of the string s to
  compare.
  @return An integer less than, equal to, or greater than zero if the substring
  of the safe string is less than, equal to, or greater than the substring of
  the string s, respectively.
  */
  inline int compare(size_t pos1, size_t count1, const char *s,
                     size_t count2) const {
    check();
    return strPtr->compare(pos1, count1, s, count2);
  }

  /**constexpr bool starts_with( std::basic_string_view<CharT,Traits> sv ) const
  noexcept;
  @brief Checks if the safe string starts with a given string.
  @param sv The string to check for.
  @return true if the safe string starts with the string sv, false otherwise.
  */
  inline bool starts_with(const std::string &sv) const {
    check();
    return strPtr->starts_with(sv);
  }

  /** constexpr bool starts_with( CharT ch ) const noexcept;
  @brief Checks if the safe string starts with a given character.
  @param ch The character to check for.
  @return True if the safe string starts with the character ch, false otherwise.
  */
  inline bool starts_with(char ch) const {
    check();
    return strPtr->starts_with(ch);
  }

  /** constexpr bool starts_with( const CharT* s ) const;
  @brief Checks if the safe string starts with a given array of characters.
  @param s The array of characters to check for.
  @return True if the safe string starts with the array of characters s, false
  otherwise.
  */
  inline bool starts_with(const char *s) const {
    check();
    return strPtr->starts_with(s);
  }

  /** constexpr bool ends_with( std::basic_string_view<CharT,Traits> sv ) const
  noexcept;
  @brief Checks if the safe string ends with a given string.
  @param sv The string to check for.
  @return True if the safe string ends with the string sv, false otherwise.
  */
  inline bool ends_with(const std::string &sv) const {
    check();
    return strPtr->ends_with(sv);
  }

  /** constexpr bool ends_with( CharT ch ) const noexcept;
  @brief Checks if the safe string ends with a given character.
  @param ch The character to check for.
  @return True if the safe string ends with the character ch, false otherwise.
  */
  inline bool ends_with(char ch) const {
    check();
    return strPtr->ends_with(ch);
  }

  /** constexpr bool ends_with( const CharT* s ) const;
  @brief Checks if the safe string ends with a given array of characters.
  @param s The array of characters to check for.
  @return True if the safe string ends with the array of characters s, false
  otherwise.
  */
  inline bool ends_with(const char *s) const {
    check();
    return strPtr->ends_with(s);
  }

  /** constexpr bool contains( std::basic_string_view<CharT,Traits> sv ) const
  noexcept; *TODO: contains (C++23) *constexpr basic_string& replace( size_type
  pos, size_type count, const basic_string& str );
  @brief Replaces a substring of the safe string with a given safe string.
  @param pos The index of the first character of the substring to replace.
  @param count The number of characters in the substring to replace.
  @param str The safe string to replace the substring with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(size_t pos, size_t count, const SafeString &str) {
    check();
    markAsUsed();
    strPtr->replace(pos, count, str.get());
    return *this;
  }

  /** constexpr basic_string& replace( size_type pos, size_type count, const
  CharT* cstr );
  @brief Replaces a substring of the safe string with a given array of
  characters.
  @param pos The index of the first character of the substring to replace.
  @param count The number of characters in the substring to replace.
  @param str The array of characters to replace the substring with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(size_t pos, size_t count, const std::string &str) {
    check();
    markAsUsed();
    strPtr->replace(pos, count, str);
    return *this;
  }

  /** constexpr basic_string& replace( const_iterator first, const_iterator
  last, const basic_string& str );
  @brief Replaces a substring of the safe string with a given safe string.
  @param first The iterator to the first character of the substring to replace.
  @param last The iterator to the last character of the substring to replace.
  @param str The safe string to replace the substring with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(std::string::const_iterator first,
                             std::string::const_iterator last,
                             const SafeString &str) {
    check();
    markAsUsed();
    strPtr->replace(first, last, str.get());
    return *this;
  }

  /** constexpr basic_string& replace( const_iterator first, const_iterator
  last, const CharT* cstr );
  @brief Replaces a substring of the safe string with a given string.
  @param first The iterator to the first character of the substring to replace.
  @param last The iterator to the last character of the substring to replace.
  @param str The string to replace the substring with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(std::string::const_iterator first,
                             std::string::const_iterator last,
                             const std::string &str) {
    check();
    markAsUsed();
    strPtr->replace(first, last, str);
    return *this;
  }

  /** constexpr basic_string& replace( size_type pos, size_type count, const
  basic_string& str, size_type pos2, size_type count2 = npos )
  @brief Replaces a substring of the safe string with a given safe string.
  @param pos The index of the first character of the substring to replace.
  @param count The number of characters in the substring to replace.
  @param str The safe string to replace the substring with.
  @param pos2 The index of the first character of the safe string to replace
  with.
  @param count2 The number of characters in the safe string to replace with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(size_t pos, size_t count, const SafeString &str,
                             size_t pos2, size_t count2 = std::string::npos) {
    check();
    markAsUsed();
    strPtr->replace(pos, count, str.get(), pos2, count2);
    return *this;
  }

  /** constexpr basic_string& replace( size_type pos, size_type count, const
  CharT* cstr, size_type count2 );
  @brief Replaces a substring of the safe string with a given string.
  @param pos The index of the first character of the substring to replace.
  @param count The number of characters in the substring to replace.
  @param str The string to replace the substring with.
  @param count2 The number of characters in the string to replace with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(size_t pos, size_t count, const std::string &str,
                             size_t pos2, size_t count2 = std::string::npos) {
    check();
    markAsUsed();
    strPtr->replace(pos, count, str, pos2, count2);
    return *this;
  }

  /** constexpr basic_string& replace( const_iterator first, const_iterator
  last, InputIt first2, InputIt last2 );
  @brief Replaces a substring of the safe string with a given range of
  iterators.
  @param first The iterator to the first character of the substring to replace.
  @param last The iterator to the last character of the substring to replace.
  @param first2 The iterator to the first character of the range to replace the
  substring with.
  @param last2 The iterator to the last character of the range to replace the
  substring with.
  @return A reference to the safe string.
  */
  template <class InputIt>
  inline SafeString &replace(std::string::const_iterator first,
                             std::string::const_iterator last, InputIt first2,
                             InputIt last2) {
    check();
    markAsUsed();
    strPtr->replace(first, last, first2, last2);
    return *this;
  }

  /** constexpr basic_string& replace( size_type pos, size_type count, const CharT* cstr, size_type count2 );
  @brief Replaces a substring of the safe string with a given array of characters.
  @param pos The index of the first character of the substring to replace.
  @param count The number of characters in the substring to replace.
  @param cstr The array of characters to replace the substring with.
  @param count2 The number of characters in the array to replace with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(size_t pos, size_t count, const char *cstr,
                             size_t count2) {
    check();
    strPtr->replace(pos, count, cstr, count2);
    markAsUsed();
    return *this;
  }

  /** constexpr basic_string& replace( const_iterator first, const_iterator last, const CharT* cstr, size_type count2 );
  @brief Replaces a substring of the safe string with a given array of characters.
  @param first The iterator to the first character of the substring to replace.
  @param last The iterator to the last character of the substring to replace.
  @param cstr The array of characters to replace the substring with.
  @param count2 The number of characters in the array to replace with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(std::string::const_iterator first,
                             std::string::const_iterator last, const char *cstr,
                             size_t count2) {
    check();
    markAsUsed();
    strPtr->replace(first, last, cstr, count2);
    return *this;
  }

  /** constexpr basic_string& replace( size_type pos, size_type count, const CharT* cstr );
  @brief Replaces a substring of the safe string with a given array of characters.
  @param pos The index of the first character of the substring to replace.
  @param count The number of characters in the substring to replace.
  @param cstr The array of characters to replace the substring with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(size_t pos, size_t count, const char *cstr) {
    check();
    markAsUsed();
    strPtr->replace(pos, count, cstr);
    return *this;
  }

  /** constexpr basic_string& replace( const_iterator first, const_iterator last, const CharT* cstr );
  @brief Replaces a substring of the safe string with a given array of characters.
  @param first The iterator to the first character of the substring to replace.
  @param last The iterator to the last character of the substring to replace.
  @param cstr The array of characters to replace the substring with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(std::string::const_iterator first,
                             std::string::const_iterator last,
                             const char *cstr) {
    check();
    markAsUsed();
    strPtr->replace(first, last, cstr);
    return *this;
  }

  /** constexpr basic_string& replace( size_type pos, size_type count, size_type count2, CharT ch );
  @brief Replaces a substring of the safe string with a given character.
  @param pos The index of the first character of the substring to replace.
  @param count The number of characters in the substring to replace.
  @param count2 The number of characters to replace the substring with.
  @param ch The character to replace the substring with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(size_t pos, size_t count, size_t count2, char ch) {
    check();
    markAsUsed();
    strPtr->replace(pos, count, count2, ch);
    return *this;
  }

  /** constexpr basic_string& replace( const_iterator first, const_iterator last, size_type count2, CharT ch );
  @brief Replaces a substring of the safe string with a given character.
  @param first The iterator to the first character of the substring to replace.
  @param last The iterator to the last character of the substring to replace.
  @param count2 The number of characters to replace the substring with.
  @param ch The character to replace the substring with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(std::string::const_iterator first,
                             std::string::const_iterator last, size_t count2,
                             char ch) {
    check();
    markAsUsed();
    strPtr->replace(first, last, count2, ch);
    return *this;
  }

  /** constexpr basic_string& replace( const_iterator first, const_iterator last, std::initializer_list<CharT> ilist );
  @brief Replaces a substring of the safe string with a given initializer list of characters.
  @param first The iterator to the first character of the substring to replace.
  @param last The iterator to the last character of the substring to replace.
  @param ilist The initializer list of characters to replace the substring with.
  @return A reference to the safe string.
  */
  inline SafeString &replace(std::string::const_iterator first,
                             std::string::const_iterator last,
                             std::initializer_list<char> ilist) {
    check();
    markAsUsed();
    strPtr->replace(first, last, ilist);
    return *this;
  }

  /** basic_string substr( size_type pos = 0, size_type count = npos ) const;
  @brief Returns a substring of the safe string.
  @param pos The index of the first character of the substring.
  @param count The number of characters in the substring.
  @return The substring of the safe string.
  */
  inline SafeString substr(size_t pos = 0,
                           size_t count = std::string::npos) const {
    check();
    return SafeString(strPtr->substr(pos, count));
  }

  /** constexpr size_type copy( CharT* dest, size_type count, size_type pos = 0) const;
  @brief Copies a substring of the safe string into a given array of characters.
  @param dest The array of characters to copy the substring into.
  @param count The number of characters to copy.
  @param pos The index of the first character of the substring to copy.
  @return The number of characters copied.
  */
  inline size_t copy(char *dest, size_t count, size_t pos = 0) const {
    check();
    return strPtr->copy(dest, count, pos);
  }

  /** void resize( size_type count );
  @brief Resizes the safe string.
  @param count The new size of the safe string.
  */
  inline void resize(size_t count) {
    check();
    markAsUsed();
    strPtr->resize(count);
  }

  /** void resize( size_type count, CharT ch );
  @brief Resizes the safe string.
  @param count The new size of the safe string.
  @param ch The character to fill the new space with.
  */
  inline void resize(size_t count, char ch) {
    check();
    markAsUsed();
    strPtr->resize(count, ch);
  }

  /** void swap( basic_string& other ) noexcept;
  @brief Swaps the contents of two safe strings.
  @param other The safe string to swap with.
  */
  inline void swap(SafeString &other) {
    check();
    other.check();
    markAsUsed();
    other.markAsUsed();
    strPtr.swap(other.strPtr);
  }

  /** constexpr size_type find( const basic_string& str, size_type pos = 0 ) const noexcept;
  @brief Finds the first occurrence of a given safe string in the safe string.
  @param str The safe string to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of the safe string.
  */
  inline size_t find(const SafeString &str, size_t pos = 0) const {
    check();
    return strPtr->find(str.get(), pos);
  }

  /** constexpr size_type find( const CharT* s, size_type pos = 0 ) const;
  @brief Finds the first occurrence of a given C string in the safe string.
  @param s The C string to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of the C string.
  */
  inline size_t find(const std::string &str, size_t pos = 0) const {
    check();
    return strPtr->find(str, pos);
  }

  /** constexpr size_type find( const CharT* s, size_type pos, size_type count ) const;
  @brief Finds the first occurrence of a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @param count The number of characters to search.
  @return The index of the first occurrence of the array of characters.
  */
  inline size_t find(const char *s, size_t pos, size_t count) const {
    check();
    return strPtr->find(s, pos, count);
  }

  /** constexpr size_type find( const CharT* s, size_type pos = 0 ) const;
  @brief Finds the first occurrence of a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of the array of characters.
  */
  inline size_t find(const char *s, size_t pos = 0) const {
    check();
    return strPtr->find(s, pos);
  }

  /** constexpr size_type find( CharT ch, size_type pos = 0 ) const noexcept;
  @brief Finds the first occurrence of a given character in the safe string.
  @param ch The character to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of the character.
  */
  inline size_t find(char ch, size_t pos = 0) const {
    check();
    return strPtr->find(ch, pos);
  }

  /** constexpr size_type rfind( const basic_string& str, size_type pos = npos ) const noexcept;
  @brief Finds the last occurrence of a given safe string in the safe string.
  @param str The safe string to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of the safe string.
  */
  inline size_t rfind(const SafeString &str,
                      size_t pos = std::string::npos) const {
    check();
    return strPtr->rfind(str.get(), pos);
  }

  /** constexpr size_type rfind( const CharT* s, size_type pos = npos ) const;
  @brief Finds the last occurrence of a given string in the safe string.
  @param s The string to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of the string.
  */
  inline size_t rfind(const std::string &str,
                      size_t pos = std::string::npos) const {
    check();
    return strPtr->rfind(str, pos);
  }

  /** constexpr size_type rfind( const CharT* s, size_type pos, size_type count) const;
  @brief Finds the last occurrence of a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @param count The number of characters to search.
  @return The index of the last occurrence of the array of characters.
  */
  inline size_t rfind(const char *s, size_t pos, size_t count) const {
    check();
    return strPtr->rfind(s, pos, count);
  }

  /** constexpr size_type rfind( const CharT* s, size_type pos = npos ) const;
  @brief Finds the last occurrence of a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of the array of characters.
  */
  inline size_t rfind(const char *s, size_t pos = std::string::npos) const {
    check();
    return strPtr->rfind(s, pos);
  }

  /** constexpr size_type rfind( CharT ch, size_type pos = npos ) const noexcept;
  @brief Finds the last occurrence of a given character in the safe string.
  @param ch The character to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of the character.
  */
  inline size_t rfind(char ch, size_t pos = std::string::npos) const {
    check();
    return strPtr->rfind(ch, pos);
  }

  /** constexpr size_type find_first_of( const basic_string& str, size_type pos = 0 ) const noexcept;
  @brief Finds the first occurrence of any of the characters in a given safe string in the safe string.
  @param str The safe string to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of any of the characters in the safe string.
  */
  inline size_t find_first_of(const SafeString &str, size_t pos = 0) const {
    check();
    return strPtr->find_first_of(str.get(), pos);
  }

  /** constexpr size_type find_first_of( const CharT* s, size_type pos = 0 ) const;
  @brief Finds the first occurrence of any of the characters in a given string in the safe string.
  @param s The string to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of any of the characters in the string.
  */
  inline size_t find_first_of(const std::string &str, size_t pos = 0) const {
    check();
    return strPtr->find_first_of(str, pos);
  }

  /** constexpr size_type find_first_of( const CharT* s, size_type pos, size_type count ) const;
  @brief Finds the first occurrence of any of the characters in a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @param count The number of characters to search.
  @return The index of the first occurrence of any of the characters in the array of characters.
  */
  inline size_t find_first_of(const char *s, size_t pos, size_t count) const {
    check();
    return strPtr->find_first_of(s, pos, count);
  }

  /** constexpr size_type find_first_of( const CharT* s, size_type pos = 0 ) const;
  @brief Finds the first occurrence of any of the characters in a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of any of the characters in the array of characters.
  */
  inline size_t find_first_of(const char *s, size_t pos = 0) const {
    check();
    return strPtr->find_first_of(s, pos);
  }

  /** constexpr size_type find_first_of( CharT ch, size_type pos = 0 ) const noexcept;
  @brief Finds the first occurrence of a given character in the safe string.
  @param ch The character to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of the character.
  */
  inline size_t find_first_of(char ch, size_t pos = 0) const {
    check();
    return strPtr->find_first_of(ch, pos);
  }

  /** size_type find_first_not_of( const basic_string& str, size_type pos = 0 ) const noexcept;
  @brief Finds the first occurrence of none of the characters in a given safe string in the safe string.
  @param str The safe string to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of none of the characters in the safe string.
  */
  inline size_t find_first_not_of(const SafeString &str, size_t pos = 0) const {
    check();
    return strPtr->find_first_not_of(str.get(), pos);
  }

  /** constexpr size_type find_first_not_of( const CharT* s, size_type pos = 0 ) const;
  @brief Finds the first occurrence of none of the characters in a given string in the safe string.
  @param s The string to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of none of the characters in the string.
  */
  inline size_t find_first_not_of(const std::string &str,
                                  size_t pos = 0) const {
    check();
    return strPtr->find_first_not_of(str, pos);
  }

  /** constexpr size_type find_first_not_of( const CharT* s, size_type pos, size_type count ) const;
  @brief Finds the first occurrence of none of the characters in a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @param count The number of characters to search.
  @return The index of the first occurrence of none of the characters in the array of characters.
  */
  inline size_t find_first_not_of(const char *s, size_t pos,
                                  size_t count) const {
    check();
    return strPtr->find_first_not_of(s, pos, count);
  }

  /** constexpr size_type find_first_not_of( const CharT* s, size_type pos = 0 ) const;
  @brief Finds the first occurrence of none of the characters in a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of none of the characters in the array of characters.
  */
  inline size_t find_first_not_of(const char *s, size_t pos = 0) const {
    check();
    return strPtr->find_first_not_of(s, pos);
  }

  /** constexpr size_type find_first_not_of( CharT ch, size_type pos = 0 ) const noexcept;
  @brief Finds the first occurrence of none of a given character in the safe string.
  @param ch The character to find.
  @param pos The index of the first character to search.
  @return The index of the first occurrence of none of the character.
  */
  inline size_t find_first_not_of(char ch, size_t pos = 0) const {
    check();
    return strPtr->find_first_not_of(ch, pos);
  }

  /** size_type find_last_of( const basic_string& str, size_type pos = npos ) const noexcept;
  @brief Finds the last occurrence of any of the characters in a given safe string in the safe string.
  @param str The safe string to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of any of the characters in the safe string.
  */
  inline size_t find_last_of(const SafeString &str,
                             size_t pos = std::string::npos) const {
    check();
    return strPtr->find_last_of(str.get(), pos);
  }

  /** constexpr size_type find_last_of( const CharT* s, size_type pos = npos ) const;
  @brief Finds the last occurrence of any of the characters in a given string in the safe string.
  @param s The string to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of any of the characters in the string.
  */
  inline size_t find_last_of(const std::string &str,
                             size_t pos = std::string::npos) const {
    check();
    return strPtr->find_last_of(str, pos);
  }

  /** constexpr size_type find_last_of( const CharT* s, size_type pos, size_type count ) const;
  @brief Finds the last occurrence of any of the characters in a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @param count The number of characters to search.
  @return The index of the last occurrence of any of the characters in the array of characters.
  */
  inline size_t find_last_of(const char *s, size_t pos, size_t count) const {
    check();
    return strPtr->find_last_of(s, pos, count);
  }

  /** constexpr size_type find_last_of( const CharT* s, size_type pos = npos ) const;
  @brief Finds the last occurrence of any of the characters in a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of any of the characters in the array of characters.
  */
  inline size_t find_last_of(const char *s,
                             size_t pos = std::string::npos) const {
    check();
    return strPtr->find_last_of(s, pos);
  }

  /** constexpr size_type find_last_of( CharT ch, size_type pos = npos ) const noexcept;
  @brief Finds the last occurrence of a given character in the safe string.
  @param ch The character to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of the character.
  */
  inline size_t find_last_of(char ch, size_t pos = std::string::npos) const {
    check();
    return strPtr->find_last_of(ch, pos);
  }

  /** size_type find_last_not_of( const basic_string& str, size_type pos = npos ) const noexcept;
  @brief Finds the last occurrence of none of the characters in a given safe string in the safe string.
  @param str The safe string to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of none of the characters in the safe string.
  */
  inline size_t find_last_not_of(const SafeString &str,
                                 size_t pos = std::string::npos) const {
    check();
    return strPtr->find_last_not_of(str.get(), pos);
  }

  /** constexpr size_type find_last_not_of( const CharT* s, size_type pos = npos ) const;
  @brief Finds the last occurrence of none of the characters in a given string in the safe string.
  @param s The string to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of none of the characters in the string.
  */
  inline size_t find_last_not_of(const std::string &str,
                                 size_t pos = std::string::npos) const {
    check();
    return strPtr->find_last_not_of(str, pos);
  }

  /** constexpr size_type find_last_not_of( const CharT* s, size_type pos, size_type count ) const;
  @brief Finds the last occurrence of none of the characters in a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @param count The number of characters to search.
  @return The index of the last occurrence of none of the characters in the array of characters.
  */
  inline size_t find_last_not_of(const char *s, size_t pos,
                                 size_t count) const {
    check();
    return strPtr->find_last_not_of(s, pos, count);
  }

  /** constexpr size_type find_last_not_of( const CharT* s, size_type pos = npos) const;
  @brief Finds the last occurrence of none of the characters in a given array of characters in the safe string.
  @param s The array of characters to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of none of the characters in the array of characters.
  */
  inline size_t find_last_not_of(const char *s,
                                 size_t pos = std::string::npos) const {
    check();
    return strPtr->find_last_not_of(s, pos);
  }

  /** constexpr size_type find_last_not_of( CharT ch, size_type pos = npos ) const noexcept;
  @brief Finds the last occurrence of none of the characters in a given character in the safe string.
  @param ch The character to find.
  @param pos The index of the first character to search.
  @return The index of the last occurrence of none of the characters in the character.
  */
  inline size_t find_last_not_of(char ch,
                                 size_t pos = std::string::npos) const {
    check();
    return strPtr->find_last_not_of(ch, pos);
  }

  /** constexpr basic_string& operator=( const basic_string& str );
  @brief Assigns a safe string to the safe string.
  @param str The safe string to assign.
  @return The safe string.
  */
  inline SafeString &operator=(const SafeString &other) {
    check();
    markAsUsed();
    *strPtr = other.get();
    return *this;
  }

  /** constexpr basic_string& operator=( basic_string&& str ) noexcept;
  @brief Assigns a string to the safe string.
  @param str The string to assign.
  @return The safe string.
  */
  inline SafeString &operator=(const std::string &other) {
    check();
    markAsUsed();
    *strPtr = other;
    return *this;
  }

  /** constexpr basic_string& operator=( const CharT* s );
  @brief Assigns an array of characters to the safe string.
  @param s The array of characters to assign.
  @return The safe string.
  */
  inline SafeString &operator=(const char *s) {
    check();
    markAsUsed();
    *strPtr = s;
    return *this;
  }

  /** constexpr basic_string& operator=( CharT ch );
  @brief Assigns a character to the safe string.
  @param ch The character to assign.
  @return The safe string.
  */
  inline SafeString &operator=(char ch) {
    check();
    markAsUsed();
    *strPtr = ch;
    return *this;
  }

  /** basic_string& operator=( std::initializer_list<CharT> ilist );
  @brief Assigns an initializer list of characters to the safe string.
  @param ilist The initializer list of characters to assign.
  @return The safe string.
  */
  inline SafeString &operator=(std::initializer_list<char> ilist) {
    check();
    markAsUsed();
    *strPtr = ilist;
    return *this;
  }

  /** constexpr basic_string& operator+=( const basic_string& str );
  @brief Appends a safe string to the safe string.
  @param str The safe string to append.
  @return The safe string.
  */
  inline SafeString &operator+=(const SafeString &str) {
    check();
    markAsUsed();
    strPtr->operator+=(str.get());
    return *this;
  }

  /** constexpr basic_string& operator+=( const basic_string&& str );
  @brief Appends a string to the safe string.
  @param str The string to append.
  @return The safe string.
  */
  inline SafeString &operator+=(const std::string &str) {
    check();
    markAsUsed();
    strPtr->operator+=(str);
    return *this;
  }

  /** constexpr basic_string& operator+=( CharT ch );]
  @brief Appends a character to the safe string.
  @param ch The character to append.
  @return The safe string.
  */
  inline SafeString &operator+=(char ch) {
    check();
    markAsUsed();
    strPtr->operator+=(ch);
    return *this;
  }

  /** constexpr basic_string& operator+=( const CharT* s );
  @brief Appends an array of characters to the safe string.
  @param s The array of characters to append.
  @return The safe string.
  */
  inline SafeString &operator+=(const char *s) {
    check();
    markAsUsed();
    strPtr->operator+=(s);
    return *this;
  }

  /** constexpr basic_string& operator+=( std::initializer_list<CharT> ilist )
  @brief Appends an initializer list of characters to the safe string.
  @param ilist The initializer list of characters to append.
  @return The safe string.
  */
  inline SafeString &operator+=(std::initializer_list<char> ilist) {
    check();
    markAsUsed();
    strPtr->operator+=(ilist);
    return *this;
  }

  /** constexpr reference operator[]( size_type pos );
  @brief Returns a reference to the character at a given index in the safe string.
  @param pos The index of the character.
  @return A reference to the character at the given index.
  */
  inline char &operator[](size_t pos) {
    check();
    markAsUsed();
    return strPtr->operator[](pos);
  }

  /** constexpr const_reference operator[]( size_type pos ) const;
  @brief Returns a reference to the character at a given index in the safe string.
  @param pos The index of the character.
  @return A reference to the character at the given index.
  */
  inline const char &operator[](size_t pos) const {
    check();
    return strPtr->operator[](pos);
  }

  /** template< class CharT, class Traits, class Alloc >
  * std::basic_string<CharT,Traits,Alloc> operator+( const
  * std::basic_string<CharT,Traits,Alloc>& lhs,const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Concatenates two safe strings.
  @param rhs The safe string to concatenate.
  @return The concatenated safe string.
  */
  inline SafeString operator+(const SafeString &rhs) const {
    check();
    return SafeString(*strPtr + rhs.get());
  };

  /** template< class CharT, class Traits, class Alloc >
  * std::basic_string<CharT,Traits,Alloc> operator+( const
  * std::basic_string<CharT,Traits,Alloc>& lhs,const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Concatenates a safe string and a string.
  @param rhs The string to concatenate.
  @return The concatenated safe string.
  */
  inline SafeString operator+(const std::string rhs) const {
    check();
    return SafeString(*strPtr + rhs);
  };

  /** template< class CharT, class Traits, class Alloc >
  * std::basic_string<CharT,Traits,Alloc> operator+( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const CharT* rhs );
  @brief Concatenates a safe string and an array of characters.
  @param rhs The array of characters to concatenate.
  @return The concatenated safe string.
  */
  inline SafeString operator+(const char *rhs) const {
    check();
    return SafeString(*strPtr + rhs);
  };

  /** template<class CharT, class Traits, class Alloc>
  * std::basic_string<CharT,Traits,Alloc> operator+( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, CharT rhs );
  @brief Concatenates a safe string and a character.
  @param rhs The character to concatenate.
  @return The concatenated safe string.
  */
  inline SafeString operator+(char rhs) const {
    check();
    return SafeString(*strPtr + rhs);
  };

  /** template< class CharT, class Traits, class Alloc > bool     =( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Compares two safe strings.
  @param rhs The safe string to compare.
  @return True if the safe strings are equal.
  */
  inline bool operator==(const SafeString &rhs) const {
    check();
    return *strPtr == rhs.get();
  };

  /** template< class CharT, class Traits, class Alloc > bool     =( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Compares a safe string and a string.
  @param rhs The string to compare.
  @return True if the safe strings are equal.
  */
  inline bool operator==(const std::string &rhs) const {
    check();
    return *strPtr == rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator==( const
  *CharT lhs, const std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Compares a character and a safe string.
  @param rhs The character to compare.
  @return True if the safe strings are equal.
  */
  inline bool operator==(const char *rhs) const {
    check();
    return *strPtr == rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator!=( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if two safe strings are not equal.
  @param rhs The safe string to compare.
  @return True if the safe strings are not equal.
  */
  inline bool operator!=(const SafeString &rhs) const {
    check();
    return *strPtr != rhs.get();
  };

  /** template< class CharT, class Traits, class Alloc > bool operator!=( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if a safe string and a string are not equal.
  @param rhs The string to compare.
  @return True if the safe strings are not equal.
  */
  inline bool operator!=(const std::string &rhs) const {
    check();
    return *strPtr != rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator!=( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const CharT* rhs );
  @brief Checks if a safe string and an array of characters are not equal.
  @param rhs The array of characters to compare.
  @return True if the safe strings are not equal.
  */
  inline bool operator!=(const char *rhs) const {
    check();
    return *strPtr != rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator<( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if a safe string is less than another safe string.
  @param rhs The safe string to compare.
  @return True if the safe string is less than the other safe string.
  */
  inline bool operator<(const SafeString &rhs) const {
    check();
    return *strPtr < rhs.get();
  };

  /** template< class CharT, class Traits, class Alloc > bool operator<( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if a safe string is less than a string.
  @param rhs The string to compare.
  @return True if the safe string is less than the string.
  */
  inline bool operator<(const std::string &rhs) const {
    check();
    return *strPtr < rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator<( const
  * CharT* lhs, const std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if a safe string is less than an array of characters.
  @param rhs The safe string to compare.
  @return True if the safe string is less than the array of characters.
  */
  inline bool operator<(const char *rhs) const {
    check();
    return *strPtr < rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator>( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if a safe string is greater than another safe string.
  @param rhs The safe string to compare.
  @return True if the safe string is greater than the other safe string.
  */
  inline bool operator>(const SafeString &rhs) const {
    check();
    return *strPtr > rhs.get();
  };

  /** template< class CharT, class Traits, class Alloc > bool operator>( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if a safe string is greater than a string.
  @param rhs The string to compare.
  @return True if the safe string is greater than the string.
  */
  inline bool operator>(const std::string &rhs) const {
    check();
    return *strPtr > rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator>( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const CharT* rhs );
  @brief Checks if a safe string is greater than an array of characters.
  @param rhs The safe string to compare.
  @return True if the safe string is greater than the array of characters.
  */
  inline bool operator>(const char *rhs) const {
    check();
    return *strPtr > rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator<=( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if a safe string is less than or equal to another safe string.
  @param rhs The safe string to compare.
  @return True if the safe string is less than or equal to the other safe
  string.
  */
  inline bool operator<=(const SafeString &rhs) const {
    check();
    return *strPtr <= rhs.get();
  };

  /** template< class CharT, class Traits, class Alloc > bool operator<=( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if a safe string is less than or equal to a string.
  @param rhs The string to compare.
  @return True if the safe string is less than or equal to the string.
  */
  inline bool operator<=(const std::string &rhs) const {
    check();
    return *strPtr <= rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator<=( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const CharT* rhs );
  @brief Checks if a safe string is less than or equal to an array of
  characters.
  @param rhs The safe string to compare.
  @return True if the safe string is less than or equal to the array of
  characters.
  */
  inline bool operator<=(const char *rhs) const {
    check();
    return *strPtr <= rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator>=( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if a safe string is greater than or equal to another safe
  string.
  @param rhs The safe string to compare.
  @return True if the safe string is greater than or equal to the other safe
  string.
  */
  inline bool operator>=(const SafeString &rhs) const {
    check();
    return *strPtr >= rhs.get();
  };

  /** template< class CharT, class Traits, class Alloc > bool operator>=( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const
  * std::basic_string<CharT,Traits,Alloc>& rhs );
  @brief Checks if a safe string is greater than or equal to a string.
  @param rhs The string to compare.
  @return True if the safe string is greater than or equal to the string.
  */
  inline bool operator>=(const std::string &rhs) const {
    check();
    return *strPtr >= rhs;
  };

  /** template< class CharT, class Traits, class Alloc > bool operator>=( const
  * std::basic_string<CharT,Traits,Alloc>& lhs, const CharT* rhs );
  @brief Checks if a safe string is greater than or equal to an array of
  characters.
  @param rhs The safe string to compare.
  @return True if the safe string is greater than or equal to the array of
  characters.
  */
  inline bool operator>=(const char *rhs) const {
    check();
    return *strPtr >= rhs;
  };

  /**
  @brief Getter for the value of the safe string.
  @return The value of the safe string.
  */
  inline const std::string &get() const {
    check();
    return *strPtr;
  };

  /**
  @brief Commit the changes to the safe string.
  */
  inline void commit() override {
    check();
    str = *strPtr;
    registered = false;
  };

  /**
  @brief Revert the changes to the safe string (nullify the pointer).
  */
  inline void revert() const override {
    strPtr = nullptr;
    registered = false;
  };
};

/**
@brief Overload of the << operator for safe strings.
@param _out The output stream to write to.
@param _t The safe string to write.
@return The output stream.
*/
inline std::ostream &operator<<(std::ostream &_out, SafeString const &_t) {
  _out << _t.get();
  return _out;
}

#endif // SAFESTRING_H