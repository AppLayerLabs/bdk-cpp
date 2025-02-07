/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef EVMCCONV_H
#define EVMCCONV_H

#include <evmc/evmc.hpp>

#include "strings.h" // Functor
#include "uintconv.h" // utils/bytes.h, bytes/view.h

/// Namespace for general EVMC-related functions.
namespace EVMCConv {
  /**
   * Wrapper for EVMC's `ecrecover()` function.
   * @param hash The hash to recover an address from.
   * @param v The recover ID.
   * @param r The first half of the ECDSA signature.
   * @param s The second half of the ECDSA signature.
   * @return The recovered address.
   */
  evmc::address ecrecover(evmc::bytes32 hash, evmc::bytes32 v, evmc::bytes32 r, evmc::bytes32 s); // TODO: not implemented(???)

  ///@{
  /**
   * Convert a given EVMC type to a BDK type, or vice-versa.
   * @param x The type to convert.
   * @return The converted type.
   */
  uint256_t evmcUint256ToUint256(const evmc::uint256be& x);
  evmc::uint256be uint256ToEvmcUint256(const uint256_t& x);
  BytesArr<32> evmcUint256ToBytes(const evmc::uint256be& x);
  evmc::uint256be bytesToEvmcUint256(const View<Bytes> x);
  ///@}

  /**
   * Get the functor of a evmc_message
   * @param msg The evmc_message to get the functor from.
   * @return The functor of the evmc_message (0 if evmc_message size == 0).
   */
  Functor getFunctor(const evmc_message& msg);

  /**
   * Get the View<Bytes> representing the function arguments of a given evmc_message.
   * @param msg The evmc_message to get the function arguments from.
   * @return The View<Bytes> representing the function arguments.
   */
  View<Bytes> getFunctionArgs(const evmc_message& msg);
};

#endif // EVMCCONV_H

