/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFETUPLE_H
#define SAFETUPLE_H

#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "safebase.h"

// Forward declarations.
template<typename... Types> class SafeTuple;
template<std::size_t I, typename... Types> decltype(auto) get(const SafeTuple<Types...>& st);
template<std::size_t I, typename... Types> decltype(auto) get(SafeTuple<Types...>& st);

/// Safe wrapper for a tuple. Used to safely store a tuple within a contract. @see SafeBase
template<typename... Types> class SafeTuple : public SafeBase {
  private:
    std::tuple<Types...> value_;  ///< Current ("original") value.
    std::tuple<std::unique_ptr<Types>...> copy_; ///< Previous ("temporary") value.

    // Helper templates to detect if a type is a std::unique_ptr or not.
    template<typename T> struct is_unique_ptr : std::false_type {};
    template<typename T> struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

    /**
     * Template for copying a specific index from one tuple to another.
     * Both tuples MUST contain the exact same quantity of items.
     * Specialization for copying from the original value to the temporary one.
     * Expects Tp2 to be unique_ptr<Tp>, which means the underlying type inside
     * the pointer MUST be the exact same as the normal type.
     * @tparam I The index to access.
     * @tparam Tp The types of the first tuple.
     * @tparam Tp2 The types of the second tuple.
     * @param from The origin tuple.
     * @param to The destination tuple.
     */
    template<std::size_t I, typename... Tp, typename... Tp2>
    void copyIndex(const std::tuple<Tp...>& from, std::tuple<Tp2...>& to)
    requires (sizeof...(Tp) == sizeof...(Tp2) && I < sizeof...(Tp) &&
      !is_unique_ptr<std::decay_t<decltype(std::get<I>(from))>>::value &&
      is_unique_ptr<std::decay_t<decltype(std::get<I>(to))>>::value &&
      std::is_same_v<std::decay_t<decltype(std::get<I>(from))>, std::decay_t<decltype(*std::get<I>(to))>>
    ) {
      if constexpr (!std::is_same_v<std::nullptr_t, std::decay_t<decltype(std::get<I>(to))>>) {
        using FromType = std::decay_t<decltype(std::get<I>(from))>;
        std::get<I>(to) = std::make_unique<FromType>(std::get<I>(from));
      }
    }

    /**
     * Template for copying a specific index from one tuple to another.
     * Both tuples MUST contain the exact same quantity of items.
     * Specialization for copying from the temporary value back to the original one.
     * Same concepts apply, except "from" and "to" are flipped now - "from" is
     * the one with unique_ptrs and "to" is the one with normal types.
     * @tparam I The index to access.
     * @tparam Tp The types of the first tuple.
     * @tparam Tp2 The types of the second tuple.
     * @param from The origin tuple.
     * @param to The destination tuple.
     */
    template<std::size_t I, typename... Tp, typename... Tp2>
    void copyIndex(const std::tuple<Tp...>& from, std::tuple<Tp2...>& to)
    requires (sizeof...(Tp) == sizeof...(Tp2) && I < sizeof...(Tp) &&
      is_unique_ptr<std::decay_t<decltype(std::get<I>(from))>>::value &&
      !is_unique_ptr<std::decay_t<decltype(std::get<I>(to))>>::value &&
      std::is_same_v<std::decay_t<decltype(*std::get<I>(from))>, std::decay_t<decltype(std::get<I>(to))>>
    ) {
      if constexpr (!std::is_same_v<std::nullptr_t, std::decay_t<decltype(std::get<I>(from))>>) {
        std::get<I>(to) = *std::get<I>(from);
      }
    }

    /**
     * Helper template to copy all indexes from one tuple to another. Leverages the previous templates.
     * @tparam Is A sequence of indexes automatically generated from the tuples.
     * @tparam Tp The types of the first tuple.
     * @tparam Tp2 The types of the second tuple.
     * @param from The origin tuple.
     * @param to The destination tuple.
     */
    template<std::size_t... Is, typename... Tp, typename... Tp2>
    void copyAll(const std::tuple<Tp...>& from, std::tuple<Tp2...>& to, std::index_sequence<Is...>) {
      (copyIndex<Is>(from, to), ...); // Call each index to be copied in sequence
    }

    /**
     * Template to copy all indexes from one tuple to another. Leverages the respective helper.
     * @tparam Tp The types of the first tuple.
     * @tparam Tp2 The types of the second tuple.
     * @param from The origin tuple.
     * @param to The destination tuple.
     */
    template<typename... Tp, typename... Tp2>
    void copyAll(const std::tuple<Tp...>& from, std::tuple<Tp2...>& to) {
      static_assert(sizeof...(Tp) == sizeof...(Tp2), "Tuples must have the same size");
      copyAll(from, to, std::index_sequence_for<Tp...>{}); // Create the sequence that will be used in Is above
    }

    /**
     * Template to set a specific index of a pointer tuple back to nullptr.
     * Expects Tp... to always be a unique_ptr.
     * @tparam I The index to access.
     * @tparam Tp The types of the tuple.
     * @param t The tuple to operate on.
     */
    template<std::size_t I, typename... Tp> void setIndexAsNullptr(std::tuple<Tp...>& t)
    requires(I < sizeof...(Tp) && is_unique_ptr<std::decay_t<decltype(std::get<I>(t))>>::value) {
      std::get<I>(t) = nullptr;
    }

    /**
     * Helper template to clear the temporary tuple (set everything to nullptr). Leverages the previous template.
     * @tparam Is A sequence of indexes automatically generated from the tuple.
     * @tparam Tp The types of the tuple.
     * @param t The tuple to operate on.
     */
    template<std::size_t... Is, typename... Tp> void clearCopy(std::tuple<Tp...>& t, std::index_sequence<Is...>) {
      ((setIndexAsNullptr<Is>(t)), ...); // Call each index to be nullified in sequence
    }

    /**
     * Template to clear a temporary tuple. Leverages the respective helper.
     * @tparam Tp The types of the tuple.
     * @param t The tuple to operate on.
     */
    template<typename... Tp> void clearCopy(std::tuple<Tp...>& t) {
      clearCopy(t, std::index_sequence_for<Tp...>{}); // Create the sequence that will be used in Is above
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
    SafeTuple() : SafeBase(nullptr), value_(), copy_() {} ///< Empty constructor.

    /**
     * Empty constructor with owner.
     * @param owner The contract that owns the variable.
     */
    SafeTuple(DynamicContract* owner) : SafeBase(owner), value_(), copy_() {}

    ///@{
    /**
     * Forward declaration constructor.
     * @param tpl The tuple to forward.
     */
    template<typename... U> SafeTuple(const std::tuple<U...>& tpl) : value_(tpl), copy_() {}
    template<typename... U> SafeTuple(std::tuple<U...>&& tpl) : value_(std::move(tpl)), copy_() {}
    ///@}

    /// Copy constructor. Copies only the CURRENT value.
    SafeTuple(const SafeTuple& other) : value_(other.value_), copy_() {}

    /// Move constructor. Moves only the CURRENT value.
    SafeTuple(SafeTuple&& other) noexcept : value_(std::move(other.value_)), copy_() {}

    /**
     * Variadic constructor.
     * @tparam U The argument types.
     * @param args The arguments to construct the tuple with.
     */
    template<typename... U> requires (
      !(... && std::is_base_of_v<SafeTuple, std::decay_t<U>>) &&
      !std::conjunction_v<std::is_same<std::pair<typename std::decay<U>::type...>, U>...>
    ) SafeTuple(U&&... args) : value_(std::forward<U>(args)...), copy_() {
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
      this->value_ = std::make_tuple(pair.first, pair.second);
    }

    /**
     * Helper template for getting a specific index from the original tuple (non-const).
     * Used by the non-member equivalent wrapper (so the value can actually be copied).
     * @tparam I The index to get.
     */
    template<std::size_t I> decltype(auto) get() {
      copyIndex<I>(this->value_, this->copy_); markAsUsed(); return std::get<I>(this->value_);
    }

    /**
     * Copy assignment operator.
     * @param other The SafeTuple to copy.
     */
    SafeTuple& operator=(const SafeTuple& other) {
      copyAll(this->value_, this->copy_); markAsUsed();
      this->value_ = other.value_; return *this;
    }

    /**
     * Move assignment operator.
     * @param other The SafeTuple to move.
     */
    SafeTuple& operator=(SafeTuple&& other) noexcept {
      copyAll(this->value_, this->copy_); markAsUsed(); other.markAsUsed();
      this->value_ = std::move(other.value_); return *this;
    }

    /**
     * Implicit conversion operator.
     * @tparam OtherTypes The types of the other SafeTuple.
     * @param other The SafeTuple to implicitly convert.
     * @return The SafeTuple as a tuple.
     */
    template<typename... OtherTypes> SafeTuple& operator=(const SafeTuple<OtherTypes...>& other) {
      copyAll(this->value_, this->copy_); markAsUsed();
      this->value_ = other.value_; return *this;
    }

    /**
     * Pair assignment operator.
     * @tparam U The first type of the pair.
     * @tparam V The second type of the pair.
     * @param pair The pair to assign.
     */
    template<typename U, typename V> SafeTuple& operator=(const std::pair<U, V>& pair) {
      static_assert(sizeof...(Types) == 2, "Tuple must have 2 elements to be assigned from a pair");
      copyAll(this->value_, this->copy_); markAsUsed();
      this->value_ = pair; return *this;
    }

    /**
     * Swap the contents of two SafeTuples. Swaps only the CURRENT value.
     * @param other The other SafeTuple to swap with.
     */
    void swap(SafeTuple& other) noexcept {
      copyAll(this->value_, this->copy_); markAsUsed(); other.markAsUsed();
      std::swap(this->value_, other.value_);
    }

    /// Commit the value.
    inline void commit() override { clearCopy(this->copy_); this->registered_ = false; }

    /// Revert the value.
    inline void revert() override { copyAll(this->copy_, this->value_); this->registered_ = false; }
};

/// Non-member get function for SafeTuple (const).
template<std::size_t I, typename... Types> decltype(auto) get(const SafeTuple<Types...>& st) {
  return std::get<I>(st.value_);
}

/// Non-member get function for SafeTuple (non-const).
template<std::size_t I, typename... Types> decltype(auto) get(SafeTuple<Types...>& st) {
  return st.template get<I>();
}

/// Non-member swap function for SafeTuple.
template<typename... Types> void swap(SafeTuple<Types...>& lhs, SafeTuple<Types...>& rhs) noexcept {
  lhs.markAsUsed(); rhs.markAsUsed(); lhs.swap(rhs);
}

/// Non-member equality operator for SafeTuple.
template<typename... LTypes, typename... RTypes>
bool operator==(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  return lhs.value_ == rhs.value_;
}

/// Non-member inequality operator for SafeTuple.
template<typename... LTypes, typename... RTypes>
bool operator!=(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  return lhs.value_ != rhs.value_;
}

/// Non-member less than operator for SafeTuple.
template<typename... LTypes, typename... RTypes>
bool operator<(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  return lhs.value_ < rhs.value_;
}

/// Non-member less than or equal to operator for SafeTuple.
template<typename... LTypes, typename... RTypes>
bool operator<=(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  return lhs.value_ <= rhs.value_;
}

/// Non-member greater than operator for SafeTuple.
template<typename... LTypes, typename... RTypes>
bool operator>(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  return lhs.value_ > rhs.value_;
}

/// Non-member greater than or equal to operator for SafeTuple.
template<typename... LTypes, typename... RTypes>
bool operator>=(const SafeTuple<LTypes...>& lhs, const SafeTuple<RTypes...>& rhs) {
  return lhs.value_ >= rhs.value_;
}

#endif // SAFETUPLE_H
