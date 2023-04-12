#ifndef SAFESTRING_H
#define SAFESTRING_H

#include <string>
#include <memory>

class SafeString {
  private:
    std::string str;
    mutable std::unique_ptr<std::string> strPtr;

    void check() const { if (strPtr == nullptr) { strPtr = std::make_unique<std::string>(str); } }
  public:
    /// Default constructor
    SafeString() : strPtr(std::make_unique<std::string>()) {}

    /// Constructor from string
    explicit SafeString(const std::string& str) : strPtr(std::make_unique<std::string>(str)) {}

    /// Copy constructor
    SafeString(const SafeString& other) {
      other.check();
      strPtr = std::make_unique<std::string>(*other.strPtr);
    }

    /// Member Functions:
    /// constexpr basic_string& assign( size_type count, CharT ch );
    inline SafeString& assign(size_t count, char ch) { check(); strPtr->assign(count, ch); return *this; }
    /// constexpr basic_string& assign( const basic_string& str );2
    inline SafeString& assign(const SafeString& str) { check(); strPtr->assign(str.get()); return *this; }
    /// constexpr basic_string& assign( const basic_string& str, size_type pos, size_type count = npos);
    inline SafeString& assign(const SafeString& str, size_t pos, size_t count = std::string::npos) { check(); strPtr->assign(str.get(), pos, count); return *this; }
    /// constexpr basic_string& assign( const CharT* s, size_type count );
    inline SafeString& assign(const char* s, size_t count) { check(); strPtr->assign(s, count); return *this; }
    /// constexpr basic_string& assign( const CharT* s );
    inline SafeString& assign(const char* s) { check(); strPtr->assign(s); return *this; }
    /// template< class InputIt >
    /// constexpr basic_string& assign( InputIt first, InputIt last );
    template<class InputIt>
    inline SafeString& assign(InputIt first, InputIt last) { check(); strPtr->assign(first, last); return *this; }
    /// constexpr basic_string& assign( std::initializer_list<CharT> ilist );
    inline SafeString& assign(std::initializer_list<char> ilist) { check(); strPtr->assign(ilist); return *this; }

    /// Element Access:
    /// constexpr reference at( size_type pos );
    inline char& at(size_t pos) { check(); return strPtr->at(pos); }
    /// constexpr const_reference at( size_type pos ) const;
    inline const char& at(size_t pos) const { check(); return strPtr->at(pos); }
    /// constexpr reference front();
    inline char& front() { check(); return strPtr->front(); }
    /// constexpr const_reference front() const;
    inline const char& front() const { check(); return strPtr->front(); }
    /// constexpr reference back();
    inline char& back() { check(); return strPtr->back(); }
    /// constexpr const_reference back() const;
    inline const char& back() const { check(); return strPtr->back(); }
    /// constexpr const CharT* c_str() const noexcept;
    inline const char* c_str() const { check(); return strPtr->c_str(); }
    /// constexpr const CharT* data() const noexcept;
    inline const char* data() const { check(); return strPtr->data(); }

    /// Iterators:
    inline std::string::iterator begin() { check(); return strPtr->begin(); }
    inline std::string::const_iterator cbegin() const { check(); return strPtr->cbegin(); }
    inline std::string::iterator end() { check(); return strPtr->end(); }
    inline std::string::const_iterator cend() const { check(); return strPtr->cend(); }
    inline std::string::reverse_iterator rbegin() { check(); return strPtr->rbegin(); }
    inline std::string::const_reverse_iterator crbegin() { check(); return strPtr->crbegin(); }
    inline std::string::reverse_iterator rend() { check(); return strPtr->rend(); }
    inline std::string::const_reverse_iterator crend() { check(); return strPtr->crend(); }

    /// Capacity:
    inline bool empty() const { check(); return strPtr->empty(); }
    inline size_t size() const { check(); return strPtr->size(); }
    inline size_t length() const { check(); return strPtr->length(); }
    inline size_t max_size() const { check(); return strPtr->max_size(); }
    inline void reserve(size_t new_cap) { check(); strPtr->reserve(new_cap); }
    inline size_t capacity() const { check(); return strPtr->capacity(); }
    inline void shrink_to_fit() { check(); strPtr->shrink_to_fit(); }

    /// Operations:
    /// constexpr void clear() noexcept;
    inline void clear() { check(); strPtr->clear(); }
    /// constexpr basic_string& insert( size_type index, size_type count, CharT ch );
    inline SafeString& insert(size_t index, size_t count, char ch) { check(); strPtr->insert(index, count, ch); return *this; }
    /// constexpr basic_string& insert( size_type index, const CharT* s );
    inline SafeString& insert(size_t index, const char* s) { check(); strPtr->insert(index, s); return *this; }
    /// constexpr basic_string& insert( size_type index, const CharT* s, size_type count );
    inline SafeString& insert(size_t index, const char* s, size_t count) { check(); strPtr->insert(index, s, count); return *this; }
    /// constexpr basic_string& insert( size_type index, const basic_string& str );
    inline SafeString& insert(size_t index, const SafeString& str) { check(); strPtr->insert(index, str.get()); return *this; }
    inline SafeString& insert(size_t index, const std::string& str) { check(); strPtr->insert(index, str); return *this; }
    /// constexpr basic_string& insert( size_type index, const basic_string& str, size_type index_str, size_type count = npos );
    inline SafeString& insert(size_t index, const SafeString& str, size_t index_str, size_t count = std::string::npos) { check(); strPtr->insert(index, str.get(), index_str, count); return *this; }
    inline SafeString& insert(size_t index, const std::string& str, size_t index_str, size_t count = std::string::npos) { check(); strPtr->insert(index, str, index_str, count); return *this; }
    /// constexpr iterator insert( const_iterator pos, CharT ch );
    inline std::string::iterator insert(std::string::const_iterator pos, char ch) { check(); return strPtr->insert(pos, ch); }
    /// constexpr iterator insert( const_iterator pos, size_type count, CharT ch );
    inline std::string::iterator insert(std::string::const_iterator pos, size_t count, char ch) { check(); return strPtr->insert(pos, count, ch); }
    /// template< class InputIt >
    /// constexpr iterator insert( const_iterator pos, InputIt first, InputIt last );
    template<class InputIt>
    inline std::string::iterator insert(std::string::const_iterator pos, InputIt first, InputIt last) { check(); return strPtr->insert(pos, first, last); }
    /// constexpr iterator insert( const_iterator pos, std::initializer_list<CharT> ilist );
    inline std::string::iterator insert(std::string::const_iterator pos, std::initializer_list<char> ilist) { check(); return strPtr->insert(pos, ilist); }
    /// constexpr basic_string& erase( size_type index = 0, size_type count = npos );
    inline SafeString& erase(size_t index = 0, size_t count = std::string::npos) { check(); strPtr->erase(index, count); return *this; }
    /// constexpr iterator erase( const_iterator position );
    inline std::string::iterator erase(std::string::const_iterator position) { check(); return strPtr->erase(position); }
    /// constexpr iterator erase( const_iterator first, const_iterator last );
    inline std::string::iterator erase(std::string::const_iterator first, std::string::const_iterator last) { check(); return strPtr->erase(first, last); }
    /// constexpr void push_back( CharT ch );
    inline void push_back(char ch) { check(); strPtr->push_back(ch); }
    /// constexpr void pop_back();
    inline void pop_back() { check(); strPtr->pop_back(); }
    /// constexpr basic_string& append( size_type count, CharT ch );
    inline SafeString& append(size_t count, char ch) { check(); strPtr->append(count, ch); return *this; }
    /// constexpr basic_string& append( const basic_string& str );
    inline SafeString& append(const SafeString& str) { check(); strPtr->append(str.get()); return *this; }
    inline SafeString& append(const std::string& str) { check(); strPtr->append(str); return *this; }
    /// constexpr basic_string& append( const basic_string& str, size_type pos, size_type count = npos )
    inline SafeString& append(const SafeString& str, size_t pos, size_t count = std::string::npos) { check(); strPtr->append(str.get(), pos, count); return *this; }
    inline SafeString& append(const std::string& str, size_t pos, size_t count = std::string::npos) { check(); strPtr->append(str, pos, count); return *this; }
    /// constexpr basic_string& append( const CharT* s, size_type count );
    inline SafeString& append(const char* s, size_t count) { check(); strPtr->append(s, count); return *this; }
    /// constexpr basic_string& append( const CharT* s );
    inline SafeString& append(const char* s) { check(); strPtr->append(s); return *this; }
    /// template< class InputIt >
    /// constexpr basic_string& append( InputIt first, InputIt last );
    template<class InputIt>
    inline SafeString& append(InputIt first, InputIt last) { check(); strPtr->append(first, last); return *this; }
    /// constexpr basic_string& append( std::initializer_list<CharT> ilist );
    inline SafeString& append(std::initializer_list<char> ilist) { check(); strPtr->append(ilist); return *this; }
    /// constexpr int compare( const basic_string& str ) const noexcept;
    inline int compare(const SafeString& str) const { check(); return strPtr->compare(str.get()); }
    inline int compare(const std::string& str) const { check(); return strPtr->compare(str); }
    /// constexpr int compare( size_type pos1, size_type count1, const basic_string& str ) const;
    inline int compare(size_t pos1, size_t count1, const SafeString& str) const { check(); return strPtr->compare(pos1, count1, str.get()); }
    inline int compare(size_t pos1, size_t count1, const std::string& str) const { check(); return strPtr->compare(pos1, count1, str); }
    /// constexpr int compare( size_type pos1, size_type count1, const basic_string& str, size_type pos2, size_type count2 = npos ) const
    inline int compare(size_t pos1, size_t count1, const SafeString& str, size_t pos2, size_t count2 = std::string::npos) const { check(); return strPtr->compare(pos1, count1, str.get(), pos2, count2); }
    inline int compare(size_t pos1, size_t count1, const std::string& str, size_t pos2, size_t count2 = std::string::npos) const { check(); return strPtr->compare(pos1, count1, str, pos2, count2); }
    /// constexpr int compare( const CharT* s ) const;
    inline int compare(const char* s) const { check(); return strPtr->compare(s); }
    /// constexpr int compare( size_type pos1, size_type count1, const CharT* s ) const;
    inline int compare(size_t pos1, size_t count1, const char* s) const { check(); return strPtr->compare(pos1, count1, s); }
    /// constexpr int compare( size_type pos1, size_type count1,const CharT* s, size_type count2 ) const;
    inline int compare(size_t pos1, size_t count1, const char* s, size_t count2) const { check(); return strPtr->compare(pos1, count1, s, count2); }
    /// constexpr bool starts_with( std::basic_string_view<CharT,Traits> sv ) const noexcept;
    inline bool starts_with(const std::string& sv) const { check(); return strPtr->starts_with(sv); }
    /// constexpr bool starts_with( CharT ch ) const noexcept;
    inline bool starts_with(char ch) const { check(); return strPtr->starts_with(ch); }
    /// constexpr bool starts_with( const CharT* s ) const;
    inline bool starts_with(const char* s) const { check(); return strPtr->starts_with(s); }
    /// constexpr bool ends_with( std::basic_string_view<CharT,Traits> sv ) const noexcept;
    inline bool ends_with(const std::string& sv) const { check(); return strPtr->ends_with(sv); }
    /// constexpr bool ends_with( CharT ch ) const noexcept;
    inline bool ends_with(char ch) const { check(); return strPtr->ends_with(ch); }
    /// constexpr bool ends_with( const CharT* s ) const;
    inline bool ends_with(const char* s) const { check(); return strPtr->ends_with(s); }
    /// constexpr bool contains( std::basic_string_view<CharT,Traits> sv ) const noexcept;
    /// TODO: contains (C++23)
    /// constexpr basic_string& replace( size_type pos, size_type count, const basic_string& str );
    inline SafeString& replace(size_t pos, size_t count, const SafeString& str) { check(); strPtr->replace(pos, count, str.get()); return *this; }
    inline SafeString& replace(size_t pos, size_t count, const std::string& str) { check(); strPtr->replace(pos, count, str); return *this; }
    /// constexpr basic_string& replace( const_iterator first, const_iterator last, const basic_string& str );
    inline SafeString& replace(std::string::const_iterator first, std::string::const_iterator last, const SafeString& str) { check(); strPtr->replace(first, last, str.get()); return *this; }
    inline SafeString& replace(std::string::const_iterator first, std::string::const_iterator last, const std::string& str) { check(); strPtr->replace(first, last, str); return *this; }
    /// constexpr basic_string& replace( size_type pos, size_type count, const basic_string& str, size_type pos2, size_type count2 = npos )
    inline SafeString& replace(size_t pos, size_t count, const SafeString& str, size_t pos2, size_t count2 = std::string::npos) { check(); strPtr->replace(pos, count, str.get(), pos2, count2); return *this; }
    inline SafeString& replace(size_t pos, size_t count, const std::string& str, size_t pos2, size_t count2 = std::string::npos) { check(); strPtr->replace(pos, count, str, pos2, count2); return *this; }
    /// template< class InputIt >
    /// constexpr basic_string& replace( const_iterator first, const_iterator last, InputIt first2, InputIt last2 );
    template<class InputIt>
    inline SafeString& replace(std::string::const_iterator first, std::string::const_iterator last, InputIt first2, InputIt last2) { check(); strPtr->replace(first, last, first2, last2); return *this; }
    /// constexpr basic_string& replace( size_type pos, size_type count, const CharT* cstr, size_type count2 );
    inline SafeString& replace(size_t pos, size_t count, const char* cstr, size_t count2) { check(); strPtr->replace(pos, count, cstr, count2); return *this; }
    /// constexpr basic_string& replace( const_iterator first, const_iterator last, const CharT* cstr, size_type count2 );
    inline SafeString& replace(std::string::const_iterator first, std::string::const_iterator last, const char* cstr, size_t count2) { check(); strPtr->replace(first, last, cstr, count2); return *this; }
    /// constexpr basic_string& replace( size_type pos, size_type count, const CharT* cstr );
    inline SafeString& replace(size_t pos, size_t count, const char* cstr) { check(); strPtr->replace(pos, count, cstr); return *this; }
    /// constexpr basic_string& replace( const_iterator first, const_iterator last, const CharT* cstr );
    inline SafeString& replace(std::string::const_iterator first, std::string::const_iterator last, const char* cstr) { check(); strPtr->replace(first, last, cstr); return *this; }
    /// constexpr basic_string& replace( size_type pos, size_type count, size_type count2, CharT ch );
    inline SafeString& replace(size_t pos, size_t count, size_t count2, char ch) { check(); strPtr->replace(pos, count, count2, ch); return *this; }
    /// constexpr basic_string& replace( const_iterator first, const_iterator last, size_type count2, CharT ch );
    inline SafeString& replace(std::string::const_iterator first, std::string::const_iterator last, size_t count2, char ch) { check(); strPtr->replace(first, last, count2, ch); return *this; }
    /// constexpr basic_string& replace( const_iterator first, const_iterator last, std::initializer_list<CharT> ilist );
    inline SafeString& replace(std::string::const_iterator first, std::string::const_iterator last, std::initializer_list<char> ilist) { check(); strPtr->replace(first, last, ilist); return *this; }
    /// basic_string substr( size_type pos = 0, size_type count = npos ) const;
    inline SafeString substr(size_t pos = 0, size_t count = std::string::npos) const { check(); return SafeString(strPtr->substr(pos, count)); }
    /// constexpr size_type copy( CharT* dest, size_type count, size_type pos = 0 ) const;
    inline size_t copy(char* dest, size_t count, size_t pos = 0) const { check(); return strPtr->copy(dest, count, pos); }
    /// void resize( size_type count );
    inline void resize(size_t count) { check(); strPtr->resize(count); }
    /// void resize( size_type count, CharT ch );
    inline void resize(size_t count, char ch) { check(); strPtr->resize(count, ch); }
    /// void swap( basic_string& other ) noexcept;
    inline void swap(SafeString& other) { check(); other.check(); strPtr.swap(other.strPtr); }
    /// constexpr size_type find( const basic_string& str, size_type pos = 0 ) const noexcept;
    inline size_t find(const SafeString& str, size_t pos = 0) const { check(); return strPtr->find(str.get(), pos); }
    inline size_t find(const std::string& str, size_t pos = 0) const { check(); return strPtr->find(str, pos); }
    /// constexpr size_type find( const CharT* s, size_type pos, size_type count ) const;
    inline size_t find(const char* s, size_t pos, size_t count) const { check(); return strPtr->find(s, pos, count); }
    /// constexpr size_type find( const CharT* s, size_type pos = 0 ) const;
    inline size_t find(const char* s, size_t pos = 0) const { check(); return strPtr->find(s, pos); }
    /// constexpr size_type find( CharT ch, size_type pos = 0 ) const noexcept;
    inline size_t find(char ch, size_t pos = 0) const { check(); return strPtr->find(ch, pos); }
    /// constexpr size_type rfind( const basic_string& str, size_type pos = npos ) const noexcept;
    inline size_t rfind(const SafeString& str, size_t pos = std::string::npos) const { check(); return strPtr->rfind(str.get(), pos); }
    inline size_t rfind(const std::string& str, size_t pos = std::string::npos) const { check(); return strPtr->rfind(str, pos); }
    /// constexpr size_type rfind( const CharT* s, size_type pos, size_type count ) const;
    inline size_t rfind(const char* s, size_t pos, size_t count) const { check(); return strPtr->rfind(s, pos, count); }
    /// constexpr size_type rfind( const CharT* s, size_type pos = npos ) const;
    inline size_t rfind(const char* s, size_t pos = std::string::npos) const { check(); return strPtr->rfind(s, pos); }
    /// constexpr size_type rfind( CharT ch, size_type pos = npos ) const noexcept;
    inline size_t rfind(char ch, size_t pos = std::string::npos) const { check(); return strPtr->rfind(ch, pos); }
    /// constexpr size_type find_first_of( const basic_string& str, size_type pos = 0 ) const noexcept;
    inline size_t find_first_of(const SafeString& str, size_t pos = 0) const { check(); return strPtr->find_first_of(str.get(), pos); }
    inline size_t find_first_of(const std::string& str, size_t pos = 0) const { check(); return strPtr->find_first_of(str, pos); }
    /// constexpr size_type find_first_of( const CharT* s, size_type pos, size_type count ) const;
    inline size_t find_first_of(const char* s, size_t pos, size_t count) const { check(); return strPtr->find_first_of(s, pos, count); }
    /// constexpr size_type find_first_of( const CharT* s, size_type pos = 0 ) const;
    inline size_t find_first_of(const char* s, size_t pos = 0) const { check(); return strPtr->find_first_of(s, pos); }
    /// constexpr size_type find_first_of( CharT ch, size_type pos = 0 ) const noexcept;
    inline size_t find_first_of(char ch, size_t pos = 0) const { check(); return strPtr->find_first_of(ch, pos); }
    /// size_type find_first_not_of( const basic_string& str, size_type pos = 0 ) const noexcept;
    inline size_t find_first_not_of(const SafeString& str, size_t pos = 0) const { check(); return strPtr->find_first_not_of(str.get(), pos); }
    inline size_t find_first_not_of(const std::string& str, size_t pos = 0) const { check(); return strPtr->find_first_not_of(str, pos); }
    /// constexpr size_type find_first_not_of( const CharT* s, size_type pos, size_type count ) const;
    inline size_t find_first_not_of(const char* s, size_t pos, size_t count) const { check(); return strPtr->find_first_not_of(s, pos, count); }
    /// constexpr size_type find_first_not_of( const CharT* s, size_type pos = 0 ) const;
    inline size_t find_first_not_of(const char* s, size_t pos = 0) const { check(); return strPtr->find_first_not_of(s, pos); }
    /// constexpr size_type find_first_not_of( CharT ch, size_type pos = 0 ) const noexcept;
    inline size_t find_first_not_of(char ch, size_t pos = 0) const { check(); return strPtr->find_first_not_of(ch, pos); }
    /// size_type find_last_of( const basic_string& str, size_type pos = npos ) const noexcept;
    inline size_t find_last_of(const SafeString& str, size_t pos = std::string::npos) const { check(); return strPtr->find_last_of(str.get(), pos); }
    inline size_t find_last_of(const std::string& str, size_t pos = std::string::npos) const { check(); return strPtr->find_last_of(str, pos); }
    /// constexpr size_type find_last_of( const CharT* s, size_type pos, size_type count ) const;
    inline size_t find_last_of(const char* s, size_t pos, size_t count) const { check(); return strPtr->find_last_of(s, pos, count); }
    /// constexpr size_type find_last_of( const CharT* s, size_type pos = npos ) const;
    inline size_t find_last_of(const char* s, size_t pos = std::string::npos) const { check(); return strPtr->find_last_of(s, pos); }
    /// constexpr size_type find_last_of( CharT ch, size_type pos = npos ) const noexcept;
    inline size_t find_last_of(char ch, size_t pos = std::string::npos) const { check(); return strPtr->find_last_of(ch, pos); }
    /// size_type find_last_not_of( const basic_string& str, size_type pos = npos ) const noexcept;
    inline size_t find_last_not_of(const SafeString& str, size_t pos = std::string::npos) const { check(); return strPtr->find_last_not_of(str.get(), pos); }
    inline size_t find_last_not_of(const std::string& str, size_t pos = std::string::npos) const { check(); return strPtr->find_last_not_of(str, pos); }
    /// constexpr size_type find_last_not_of( const CharT* s, size_type pos, size_type count ) const;
    inline size_t find_last_not_of(const char* s, size_t pos, size_t count) const { check(); return strPtr->find_last_not_of(s, pos, count); }
    /// constexpr size_type find_last_not_of( const CharT* s, size_type pos = npos ) const;
    inline size_t find_last_not_of(const char* s, size_t pos = std::string::npos) const { check(); return strPtr->find_last_not_of(s, pos); }
    /// constexpr size_type find_last_not_of( CharT ch, size_type pos = npos ) const noexcept;
    inline size_t find_last_not_of(char ch, size_t pos = std::string::npos) const { check(); return strPtr->find_last_not_of(ch, pos); }

    /// Operators:
    /// constexpr basic_string& operator=( const basic_string& str );
    inline SafeString& operator=(const SafeString& other) { check(); *strPtr = other.get(); return *this; }
    inline SafeString& operator=(const std::string& other) { check(); *strPtr = other; return *this; }
    /// constexpr basic_string& operator=( const CharT* s );
    inline SafeString& operator=(const char* s) { check(); *strPtr = s; return *this; }
    /// constexpr basic_string& operator=( CharT ch );
    inline SafeString& operator=(char ch) { check(); *strPtr = ch; return *this; }
    /// basic_string& operator=( std::initializer_list<CharT> ilist );
    inline SafeString& operator=(std::initializer_list<char> ilist) { check(); *strPtr = ilist; return *this; }
    /// constexpr basic_string& operator+=( const basic_string& str );
    inline SafeString& operator+=(const SafeString& str) { check(); strPtr->operator+=(str.get()); return *this; }
    inline SafeString& operator+=(const std::string& str) { check(); strPtr->operator+=(str); return *this; }
    /// constexpr basic_string& operator+=( CharT ch );
    inline SafeString& operator+=(char ch) { check(); strPtr->operator+=(ch); return *this; }
    /// constexpr basic_string& operator+=( const CharT* s );
    inline SafeString& operator+=(const char* s) { check(); strPtr->operator+=(s); return *this; }
    /// constexpr basic_string& operator+=( std::initializer_list<CharT> ilist )
    inline SafeString& operator+=(std::initializer_list<char> ilist) { check(); strPtr->operator+=(ilist); return *this; }
    /// constexpr reference operator[]( size_type pos );
    inline char& operator[](size_t pos) { check(); return strPtr->operator[](pos); }
    /// constexpr const_reference operator[]( size_type pos ) const;
    inline const char& operator[](size_t pos) const { check(); return strPtr->operator[](pos); }
    /// template< class CharT, class Traits, class Alloc >
    /// std::basic_string<CharT,Traits,Alloc> operator+( const std::basic_string<CharT,Traits,Alloc>& lhs,const std::basic_string<CharT,Traits,Alloc>& rhs );
    inline SafeString operator+(const SafeString& rhs) { check(); return SafeString(*strPtr + rhs.get()); };
    inline SafeString operator+(const std::string rhs) { check(); return SafeString(*strPtr + rhs); };
    /// template< class CharT, class Traits, class Alloc > std::basic_string<CharT,Traits,Alloc> operator+( const std::basic_string<CharT,Traits,Alloc>& lhs, const CharT* rhs );
    inline SafeString operator+(const char* rhs) { check(); return SafeString(*strPtr + rhs); };
    /// template<class CharT, class Traits, class Alloc> std::basic_string<CharT,Traits,Alloc> operator+( const std::basic_string<CharT,Traits,Alloc>& lhs, CharT rhs );
    inline SafeString operator+(char rhs) { check(); return SafeString(*strPtr + rhs); };
    /// template< class CharT, class Traits, class Alloc > bool     =( const std::basic_string<CharT,Traits,Alloc>& lhs, const std::basic_string<CharT,Traits,Alloc>& rhs );
    inline bool operator==(const SafeString& rhs) { check(); return *strPtr == rhs.get(); };
    inline bool operator==(const std::string& rhs) { check(); return *strPtr == rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator==( const CharT lhs, const std::basic_string<CharT,Traits,Alloc>& rhs );
    inline bool operator==(const char* rhs) { check(); return *strPtr == rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator!=( const std::basic_string<CharT,Traits,Alloc>& lhs, const std::basic_string<CharT,Traits,Alloc>& rhs );
    inline bool operator!=(const SafeString& rhs) { check(); return *strPtr != rhs.get(); };
    inline bool operator!=(const std::string& rhs) { check(); return *strPtr != rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator!=( const std::basic_string<CharT,Traits,Alloc>& lhs, const CharT* rhs );
    inline bool operator!=(const char* rhs) { check(); return *strPtr != rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator<( const std::basic_string<CharT,Traits,Alloc>& lhs, const std::basic_string<CharT,Traits,Alloc>& rhs );
    inline bool operator<(const SafeString& rhs) { check(); return *strPtr < rhs.get(); };
    inline bool operator<(const std::string& rhs) { check(); return *strPtr < rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator<( const CharT* lhs, const std::basic_string<CharT,Traits,Alloc>& rhs );
    inline bool operator<(const char* rhs) { check(); return *strPtr < rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator>( const std::basic_string<CharT,Traits,Alloc>& lhs, const std::basic_string<CharT,Traits,Alloc>& rhs );
    inline bool operator>(const SafeString& rhs) { check(); return *strPtr > rhs.get(); };
    inline bool operator>(const std::string& rhs) { check(); return *strPtr > rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator>( const std::basic_string<CharT,Traits,Alloc>& lhs, const CharT* rhs );
    inline bool operator>(const char* rhs) { check(); return *strPtr > rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator<=( const std::basic_string<CharT,Traits,Alloc>& lhs, const std::basic_string<CharT,Traits,Alloc>& rhs );
    inline bool operator<=(const SafeString& rhs) { check(); return *strPtr <= rhs.get(); };
    inline bool operator<=(const std::string& rhs) { check(); return *strPtr <= rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator<=( const std::basic_string<CharT,Traits,Alloc>& lhs, const CharT* rhs );
    inline bool operator<=(const char* rhs) { check(); return *strPtr <= rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator>=( const std::basic_string<CharT,Traits,Alloc>& lhs, const std::basic_string<CharT,Traits,Alloc>& rhs );
    inline bool operator>=(const SafeString& rhs) { check(); return *strPtr >= rhs.get(); };
    inline bool operator>=(const std::string& rhs) { check(); return *strPtr >= rhs; };
    /// template< class CharT, class Traits, class Alloc > bool operator>=( const std::basic_string<CharT,Traits,Alloc>& lhs, const CharT* rhs );
    inline bool operator>=(const char* rhs) { check(); return *strPtr >= rhs; };

    inline const std::string& get() const { check(); return *strPtr; };
    inline void commit() { check(); str = *strPtr; };
    inline void revert() { strPtr = nullptr; };
};

inline std::ostream& operator<<(std::ostream& _out, SafeString const& _t) {
    _out << _t.get();
    return _out;
}






#endif // SAFESTRING_H