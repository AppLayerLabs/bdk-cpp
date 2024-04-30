/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DEXV2LIBRARY_H
#define DEXV2LIBRARY_H

#include "../../../utils/utils.h"

/// Forward Declaration.
class ContractHost;

/// Namespace for common functions used in the DEXV2 contract.
namespace DEXV2Library {
  /**
   * Returns sorted token addresses, used to handle return values from
   * pairs sorted in this order.
   * Solidity counterpart: function sortTokens(address tokenA, address tokenB) internal pure returns (address token0, address token1)
   * @param tokenA The address of tokenA.
   * @param tokenB The address of tokenB.
   * @return token0, token1 (lower/higher)
   */
  std::pair<Address, Address> sortTokens(const Address& tokenA, const Address& tokenB);

  /**
   * Returns the pair address for the given tokens.
   * Differently from solidity, we don't calculate the address, we ask the factory.
   * Because we don't use CREATE2 Derivation method.
   * @param host The contract host.
   * @param factory The factory address.
   * @param tokenA The address of tokenA.
   * @param tokenB The address of tokenB.
   * @return The pair address.
   */
  Address pairFor(
    const ContractHost* host, const Address& factory,
    const Address& tokenA, const Address& tokenB
  );

  /**
   * Fetches and sorts the reserves for a pair.
   * @param host The contract host.
   * @param factory The factory address.
   * @param tokenA The address of tokenA.
   * @param tokenB The address of tokenB.
   * @return The pair of reserves.
   */
  std::pair<uint256_t, uint256_t> getReserves(
    const ContractHost* host, const Address& factory,
    const Address& tokenA, const Address& tokenB
  );

  /**
   * Given some amount of an asset and pair reserves, returns an equivalent amount of the other asset.
   * Solidity counterpart: function quote(uint amountA, uint reserveA, uint reserveB) internal pure returns (uint amountB)
   * @param amountA The amount of assetA.
   * @param reserveA The reserve of assetA.
   * @param reserveB The reserve of assetB.
   * @return The amount of assetB.
   */
  uint256_t quote(const uint256_t& amountA, const uint256_t& reserveA, const uint256_t& reserveB);

  /**
   * Given an input amount of an asset and pair reserves, returns the maximum output amount of the other asset.
   * Solidity counterpart: function getAmountOut(uint amountIn, uint reserveIn, uint reserveOut) internal pure returns (uint amountOut)
   * @param amountIn The amount of assetIn.
   * @param reserveIn The reserve of assetIn.
   * @param reserveOut The reserve of assetOut.
   * @return The amount of assetOut.
   */
  uint256_t getAmountOut(const uint256_t& amountIn, const uint256_t& reserveIn, const uint256_t& reserveOut);

  /**
   * Given an output amount of an asset and pair reserves, returns a required input amount of the other asset.
   * @param amountOut The amount of assetOut.
   * @param reserveIn The reserve of assetIn.
   * @param reserveOut The reserve of assetOut.
   * @return The amount of assetIn.
   * Solidity counterpart: function getAmountIn(uint amountOut, uint reserveIn, uint reserveOut) internal pure returns (uint amountIn)
   */
  uint256_t getAmountIn(const uint256_t& amountOut, const uint256_t& reserveIn, const uint256_t& reserveOut);

  /**
   * Performs a chained getAmountOut calculation on any number of pairs.
   * Solidity counterpart: function getAmountsOut(uint amountIn, address[] memory path) internal view returns (uint[] memory amounts)
   * @param host The contract host.
   * @param factory The factory address.
   * @param amountIn The amount of assetIn.
   * @param path The path of the pairs.
   * @return The amount each iteration will return.
   */
  std::vector<uint256_t> getAmountsOut(
    const ContractHost* host, const Address& factory,
    const uint256_t& amountIn, const std::vector<Address>& path
  );

  /**
   * Performs a chained getAmountIn calculation on any number of pairs.
   * Solidity counterpart: function getAmountsIn(uint amountOut, address[] memory path) internal view returns (uint[] memory amounts)
   * @param host The contract host.
   * @param factory The factory address.
   * @param amountOut The amount of assetOut.
   * @param path The path of the pairs.
   * @return The amount each iteration will return.
   */
   std::vector<uint256_t> getAmountsIn(
     const ContractHost* host, const Address& factory,
     const uint256_t& amountOut, const std::vector<Address>& path
   );
}

#endif  /// DEXV2LIBRARY_H
