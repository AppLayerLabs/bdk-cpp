/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STRINGS_H
#define STRINGS_H

#include <string>
#include <openssl/rand.h>
#include <span>
#include <algorithm>

#include <evmc/evmc.hpp>
#include "hex.h"
#include "bytes/range.h"
#include "bytes/initializer.h"
#include "zpp_bits.h"

#include "fixedbytes.h"
#include "address.h"
#include "hash.h"
#include "signature.h"

using StorageKey = std::pair<Address, Hash>;
using StorageKeyView = std::pair<View<Address>, View<Hash>>;


/// Abstraction of a functor (the first 4 bytes of a function's keccak hash).
struct Functor {
  uint32_t value = 0;
  inline bool operator==(const Functor& other) const { return this->value == other.value; }
};

#endif  // STRINGS_H
