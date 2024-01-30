/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFETUPLE_H
#define SAFETUPLE_H

#include <memory>
#include <tuple>

#include "safebase.h"

template<typename... Types> class SafeTuple; ///< Forward declaration of SafeTuple. @see SafeTuple

/// Non-member get function for SafeTuple (const). @see SafeTuple
template<std::size_t I, typename... Types> decltype(auto) get(const SafeTuple<Types...>& st);

/// Non-member get function for SafeTuple (non-const). @see SafeTuple
template<std::size_t I, typename... Types> decltype(auto) get(SafeTuple<Types...>& st);

/// Safe wrapper for a tuple. Used to safely store a tuple within a contract. @see SafeBase
template<typename... Types> class SafeTuple : public SafeBase {
  private:
    std::tuple<Types...> tuple_; ///< The tuple to store.
    mutable std::unique_ptr<std::tuple<Types...>> tuplePtr_; ///< The pointer to the tuple.
    std::tuple<Types...> committedTuple_; ///< Tracks the committed tuple.

    /// Check if the pointer is initialized (and initialize it if not).
    inline void check() const {
      if (tuplePtr_ == nullptr) tuplePtr_ = std::make_unique<std::tuple<Types...>>(tuple_);
    }

    /// Friend get declaration for access to private members.
    template<std::size_t I, typename... OtherTypes>
    friend decltype(auto) get(const SafeTuple<OtherTypes...>& st);

    /// Friend get declaration for access to private members.
    template<std::size_t I, typename... OtherTypes>
    friend decltype(auto) get(SafeTuple<OtherTypes...>& st);

    /// Friend swap declaration for access to private members.
    template<typename... SwapTypes>
    friend void swap(SafeTuple<SwapTypes...>& lhs, SafeTuple<SwapTypes...>& rhs) noexcept;

    /// Friend equality operator declaration for access to private members.
    template<typename... LTypes, typename... RTypes>
    friend bool operator==(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs);

    /// Friend inequality operator declaration for access to private members.
    template<typename... LTypes, typename... RTypes>
    friend bool operator!=(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs);

    /// Friend less than operator declaration for access to private members.
    template<typename... LTypes, typename... RTypes>
    friend bool operator<(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs);

    /// Friend less than or equal to operator declaration for access to private members.
    template<typename... LTypes, typename... RTypes>
    friend bool operator<=(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs);

    /// Friend greater than operator declaration for access to private members.
    template<typename... LTypes, typename... RTypes>
    friend bool operator>(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs);

    /// Friend greater than or equal to operator declaration for access to private members.
    template<typename... LTypes, typename... RTypes>
    friend bool operator>=(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs);

    /// Friend variadic constructor declaration for access to private members.
    template <typename... OtherTypes> friend class SafeTuple;

  public:
    SafeTuple() : tuple_() {} ///< Default constructor.

    /**
     * Forward declaration constructor.
     * @param tpl The tuple to forward.
     */
    template<typename... U> SafeTuple(const std::tuple<U...>& tpl) : tuple_(tpl) {}

    /**
     * Forward declaration constructor.
     * @param tpl The tuple to forward.
     */
    template<typename... U> SafeTuple(std::tuple<U...>&& tpl) : tuple_(std::move(tpl)) {}

    /**
     * Copy constructor.
     * @param other The SafeTuple to copy.
     */
    SafeTuple(const SafeTuple& other) {
      other.check();
      tuple_ = other.tuple_;
      tuplePtr_ = std::make_unique<std::tuple<Types...>>(*other.tuplePtr_);
    }

    /**
     * Move constructor.
     * @param other The SafeTuple to move.
     */
    SafeTuple(SafeTuple&& other) noexcept {
      other.check();
      tuple_ = std::move(other.tuple_);
      tuplePtr_ = std::make_unique<std::tuple<Types...>>(*other.tuplePtr_);
    }

    /**
     * Variadic constructor.
     * @tparam U The argument types.
     * @param args The arguments to construct the tuple with.
     */
    template<
      typename... U,
      typename = std::enable_if_t<
        !(... && std::is_base_of_v<SafeTuple, std::decay_t<U>>) &&
        !std::conjunction_v<std::is_same<std::pair<typename std::decay<U>::type...>, U>...>
      >
    > SafeTuple(U&&... args) : tuple_(std::forward<U>(args)...) {
      check();
      static_assert(sizeof...(U) == sizeof...(Types), "Number of arguments must match tuple size.");
    }

    /**
     * From pair constructor.
     * @tparam U The first type of the pair.
     * @tparam V The second type of the pair.
     * @param pair The pair to construct the tuple with.
     */
    template<typename U, typename V> SafeTuple(const std::pair<U, V>& pair) {
      static_assert(sizeof...(Types) == 2, "Tuple must have 2 elements to be constructed from a pair");
      tuple_ = std::make_tuple(pair.first, pair.second);
    }

    /**
     * Copy assignment operator.
     * @param other The SafeTuple to copy.
     */
    SafeTuple& operator=(const SafeTuple& other) {
      check();
      markAsUsed();
      if (&other == this) return *this;
      tuplePtr_ = std::make_unique<std::tuple<Types...>>(
        ((other.tuplePtr_) ? *other.tuplePtr_ : other.tuple_)
      );
      return *this;
    }

    /**
     * Move assignment operator.
     * @param other The SafeTuple to move.
     */
    SafeTuple& operator=(SafeTuple&& other) noexcept {
      check();
      markAsUsed();
      tuplePtr_ = std::move(other.tuplePtr_);
      if (!tuplePtr_) tuplePtr_ = std::make_unique<std::tuple<Types...>>(std::move(other.tuple_));
      return *this;
    }

    /**
     * Implicit conversion operator.
     * @tparam OtherTypes The types of the other SafeTuple.
     * @param other The SafeTuple to implicitly convert.
     * @return The SafeTuple as a tuple.
     */
    template<typename... OtherTypes> SafeTuple& operator=(const SafeTuple<OtherTypes...>& other) {
      check();
      markAsUsed();
      tuplePtr_ = std::make_unique<std::tuple<Types...>>(
        ((other.tuplePtr_) ? *other.tuplePtr_ : other.tuple_)
      );
      return *this;
    }

    /**
     * Pair assignment operator.
     * @tparam U The first type of the pair.
     * @tparam V The second type of the pair.
     * @param pair The pair to assign.
     */
    template<typename U, typename V> SafeTuple& operator=(const std::pair<U, V>& pair) {
      check();
      markAsUsed();
      static_assert(sizeof...(Types) == 2, "Tuple must have 2 elements to be assigned from a pair");
      tuplePtr_ = std::make_unique<std::tuple<Types...>>(pair.first, pair.second);
      return *this;
    }

    /**
     * Swap the contents of two SafeTuples.
     * @param other The other SafeTuple to swap with.
     */
    void swap(SafeTuple& other) noexcept {
      check();
      other.check();
      markAsUsed();
      other.markAsUsed();
      std::swap(tuple_, other.tuple_);
      std::swap(tuplePtr_, other.tuplePtr_);
    }

    /// Commit the value. Updates the value from the pointer and nullifies it.
    inline void commit() override { check(); tuple_ = *tuplePtr_; tuplePtr_ = nullptr; }

    /// Revert the value. Nullifies the pointer.
    inline void revert() const override { tuplePtr_ = nullptr; }
};

/// Non-member get function for SafeTuple (const). @see SafeTuple
template<std::size_t I, typename... Types> decltype(auto) get(const SafeTuple<Types...>& st) {
  st.check(); return std::get<I>(*st.tuplePtr_);
}

/// Non-member get function for SafeTuple (non-const). @see SafeTuple
template<std::size_t I, typename... Types> decltype(auto) get(SafeTuple<Types...>& st) {
  st.check(); return std::get<I>(*st.tuplePtr_);
}

/// Non-member swap function for SafeTuple. @see SafeTuple
template<typename... Types> void swap(SafeTuple<Types...>& lhs, SafeTuple<Types...>& rhs) noexcept {
  lhs.check(); rhs.check(); lhs.markAsUsed(); rhs.markAsUsed(); lhs.swap(rhs);
}

/// Non-member equality operator for SafeTuple. @see SafeTuple
template<typename... LTypes, typename... RTypes>
bool operator==(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  lhs.check(); rhs.check(); return lhs.tuple_ == rhs.tuple_;
}

/// Non-member inequality operator for SafeTuple. @see SafeTuple
template<typename... LTypes, typename... RTypes>
bool operator!=(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  lhs.check(); rhs.check(); return lhs.tuple_ != rhs.tuple_;
}

/// Non-member less than operator for SafeTuple. @see SafeTuple
template<typename... LTypes, typename... RTypes>
bool operator<(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  lhs.check(); rhs.check(); return lhs.tuple_ < rhs.tuple_;
}

/// Non-member less than or equal to operator for SafeTuple. @see SafeTuple
template<typename... LTypes, typename... RTypes>
bool operator<=(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  lhs.check(); rhs.check(); return lhs.tuple_ <= rhs.tuple_;
}


/// Non-member greater than operator for SafeTuple. @see SafeTuple
template<typename... LTypes, typename... RTypes>
bool operator>(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  lhs.check(); rhs.check(); return lhs.tuple_ > rhs.tuple_;
}

/// Non-member greater than or equal to operator for SafeTuple. @see SafeTuple
template<typename... LTypes, typename... RTypes>
bool operator>=(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  lhs.check(); rhs.check(); return lhs.tuple_ >= rhs.tuple_;
}

#endif // SAFETUPLE_H
