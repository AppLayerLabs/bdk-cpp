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

/**
 * Safe wrapper for a tuple. Used to safely store a tuple within a contract.
 * Used to safely store a string within a contract.
 * @see SafeBase
 */
template<typename... Types>
class SafeTuple : public SafeBase {
    private:
        std::tuple<Types...> tuple_; ///< The tuple to store.
        mutable std::unique_ptr<std::tuple<Types...>> tuplePtr_; ///< The pointer to the tuple.

        inline void check() const {
            if (tuplePtr_ == nullptr) {
                tuplePtr_ = std::make_unique<std::tuple<Types...>>(tuple_);
            }
        }

    public:

        /**
        * Default constructor.
        */
        SafeTuple() = tuple_() {};

        /**
        * Copy constructor.
        * @param other The other SafeTuple to copy.
        */
        SafeTuple(const SafeTuple& other){
            other.check();
            tuple_ = other.tuple_;
            tuplePtr_ = std::make_unique<std::tuple<Types...>>(*other.tuplePtr_);
        }

        /**
        * Move constructor.
        * @param other The other SafeTuple to move.
        */
        SafeTuple(SafeTuple&& other) noexcept {
            other.check();
            tuple_ = std::move(other.tuple_);
            tuplePtr_ = std::make_unique<std::tuple<Types...>>(*other.tuplePtr_);
        }

        /**
        * Implicit conversion constructor.
        * @param other The other SafeTuple to copy.
        */
        template<typename... OtherTypes>
        SafeTuple(const SafeTuple<OtherTypes...>& other) {
            other.check();
            tuple_ = other.tuple_;
            tuplePtr_ = std::make_unique<std::tuple<Types...>>(*other.tuplePtr_);
        }

        /**
        * Inicialization constructor.
        * @param tuple The tuple to store.
        */
        template<typename... U>
        SafeTuple(U&&... args) : tuple_(std::forward<U>(args)...) {
        static_assert(std::is_same<std::tuple<Types...>, std::tuple<U...>>::value, "Type mismatch in arguments");
        }

        /**
        * Pair conversion constructor.
        * @param pair The pair to store.
        */
        template<typename U, typename V>
        SafeTuple(const std::pair<U, V>& pair){
            static_assert(sizeof...(Types) == 2, "Tuple must have 2 elements to be constructed from a pair");
            tuple_ = std::make_tuple(pair.first, pair.second);
        }
};

#endif // SAFETUPLE_H




