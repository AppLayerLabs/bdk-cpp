/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "evmcconv.h"
#include "uintconv.h"

uint256_t EVMCConv::evmcUint256ToUint256(const evmc::uint256be& x) {
  // We can use the uint256ToBytes directly as it is std::span and we can create a span from an array
  return UintConv::bytesToUint256(View<Bytes>(x.bytes, 32));
}

evmc::uint256be EVMCConv::uint256ToEvmcUint256(const uint256_t& x) {
  // Convert the uint256_t to BytesArr<32> then copy it to evmc::uint256be
  // evmc::uint256be is a struct with a single member, bytes, which holds a uint256 value in *big-endian* order
  evmc::uint256be ret;
  BytesArr<32> bytes = UintConv::uint256ToBytes(x);
  std::copy(bytes.begin(), bytes.end(), ret.bytes);
  return ret;
}

BytesArr<32> EVMCConv::evmcUint256ToBytes(const evmc::uint256be& x) {
  BytesArr<32> ret;
  std::copy(x.bytes, x.bytes + 32, ret.begin());
  return ret;
}

evmc::uint256be EVMCConv::bytesToEvmcUint256(const View<Bytes> x) {
  evmc::uint256be ret;
  std::copy(x.begin(), x.end(), ret.bytes);
  return ret;
}

Functor EVMCConv::getFunctor(const evmc_message& msg) {
  Functor ret;
  if (msg.input_size < 4) return ret;
  // Memcpy the first 4 bytes from the input data to the function signature
  ret.value = UintConv::bytesToUint32(View<Bytes>(msg.input_data, 4));
  return ret;
}

View<Bytes> EVMCConv::getFunctionArgs(const evmc_message& msg) {
  if (msg.input_size < 4) return View<Bytes>();
  return View<Bytes>(msg.input_data + 4, msg.input_size - 4);
}

