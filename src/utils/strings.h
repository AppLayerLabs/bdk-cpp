/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STRINGS_H
#define STRINGS_H

#include <evmc/evmc.hpp> // evmc/hex.hpp -> string
#include <openssl/rand.h>

#include "../libs/zpp_bits.h" // algorithm

#include "../bytes/initializer.h" // bytes/view.h -> bytes/range.h -> ranges -> span

#include "dynamicexception.h"
#include "hex.h"
#include "uintconv.h"

#include "fixedbytes.h"
#include "address.h"
#include "hash.h"
#include "signature.h"

using StorageKey = std::pair<Address, Hash>;
using StorageKeyView = std::pair<View<Address>, View<Hash>>;


/// Abstraction of a functor (the first 4 bytes of a function's keccak hash).
struct Functor {
  uint32_t value = 0; ///< The value of the hash.

  /// Equality operator.
  inline bool operator==(const Functor& other) const { return this->value == other.value; }
};

#endif  // STRINGS_H
