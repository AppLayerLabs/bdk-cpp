/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "dexv2library.h"
#include "dexv2factory.h"
#include "dexv2pair.h"
#include "../../contractmanager.h"

namespace DEXV2Library {
  std::pair<Address, Address> sortTokens(const Address& tokenA, const Address& tokenB) {
    if (tokenA == tokenB) throw DynamicException("DEXV2Library: IDENTICAL_ADDRESSES");
    auto ret = tokenA < tokenB ? std::make_pair(tokenA, tokenB) : std::make_pair(tokenB, tokenA);
    if (ret.first == Address()) throw DynamicException("DEXV2Library: ZERO_ADDRESS");
    return ret;
  }

  Address pairFor(
    const ContractHost* host, const Address& factory,
    const Address& tokenA, const Address& tokenB
  ) {
    return host->getContract<DEXV2Factory>(factory)->getPair(tokenA, tokenB);
  }

  std::pair<uint256_t, uint256_t> getReserves(
    const ContractHost* host, const Address& factory,
    const Address& tokenA, const Address& tokenB
  ) {
    if (host == nullptr) throw DynamicException("DEXV2Library: INVALID_HOST");
    auto pair = pairFor(host, factory, tokenA, tokenB);
    std::pair<uint256_t, uint256_t> ret;
    const auto& [reserveA, reserveB, timestamp] = host->getContract<DEXV2Pair>(pair)->getReserves();
    ret.first = reserveA;
    ret.second = reserveB;
    return ret;
  }

  uint256_t quote(const uint256_t& amountA, const uint256_t& reserveA, const uint256_t& reserveB) {
    if (amountA == 0) throw DynamicException("DEXV2Library: INSUFFICIENT_AMOUNT");
    if (reserveA == 0 || reserveB == 0) throw DynamicException("DEXV2Library: INSUFFICIENT_LIQUIDITY");
    return amountA * reserveB / reserveA;
  }

  uint256_t getAmountOut(const uint256_t& amountIn, const uint256_t& reserveIn, const uint256_t& reserveOut) {
    if (amountIn == 0) throw DynamicException("DEXV2Library: INSUFFICIENT_INPUT_AMOUNT");
    if (reserveIn == 0 || reserveOut == 0) throw DynamicException("DEXV2Library: INSUFFICIENT_LIQUIDITY");
    uint256_t amountInWithFee = amountIn * 997;
    uint256_t numerator = amountInWithFee * reserveOut;
    uint256_t denominator = reserveIn * 1000 + amountInWithFee;
    return numerator / denominator;
  }

  uint256_t getAmountIn(const uint256_t& amountOut, const uint256_t& reserveIn, const uint256_t& reserveOut) {
    if (amountOut == 0) throw DynamicException("DEXV2Library: INSUFFICIENT_OUTPUT_AMOUNT");
    if (reserveIn == 0 || reserveOut == 0) throw DynamicException("DEXV2Library: INSUFFICIENT_LIQUIDITY");
    uint256_t numerator = reserveIn * amountOut * 1000;
    uint256_t denominator = (reserveOut - amountOut) * 997;
    return (numerator / denominator) + 1;
  }

  std::vector<uint256_t> getAmountsOut(
    const ContractHost* host, const Address& factory,
    const uint256_t& amountIn, const std::vector<Address>& path
  ) {
    if (path.size() < 2) throw DynamicException("DEXV2Library: INVALID_PATH");
    std::vector<uint256_t> amounts(path.size());
    amounts[0] = amountIn;
    for (size_t i = 0; i < path.size() - 1; i++) {
      auto [reservesA, reservesB] = getReserves(host, factory, path[i], path[i + 1]);
      amounts[i + 1] = getAmountOut(amounts[i], reservesA, reservesB);
    }
    return amounts;
  }

  std::vector<uint256_t> getAmountsIn(
    const ContractHost* host, const Address& factory,
    const uint256_t& amountOut, const std::vector<Address>& path
  ) {
    if (path.size() < 2) throw DynamicException("DEXV2Library: INVALID_PATH");
    std::vector<uint256_t> amounts(path.size());
    amounts[amounts.size() - 1] = amountOut;
    for (size_t i = path.size() - 1; i > 0; i--) {
      auto [reservesA, reservesB] = getReserves(host, factory, path[i - 1], path[i]);
      amounts[i - 1] = getAmountIn(amounts[i], reservesA, reservesB);
    }
    return amounts;
  }
}

