/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_DECODING_H
#define JSONRPC_DECODING_H

#include <regex>

#include "../../../utils/utils.h"
#include "../../../utils/strings.h"
#include "../../../utils/tx.h"

#include "../../../contract/contract.h"

#include "methods.h"

// Forward declarations.
class Storage;

/**
 * Namespace for decoding JSON-RPC data.
 * All functions require a JSON object that is the request itself to be operated on.
 */
namespace JsonRPC::Decoding {
  /**
   * Helper function to check if a given JSON-RPC request is valid.
   * Does NOT check if the method called is valid, only if the request follows JSON-RPC 2.0 spec.
   * @param request The request object.
   * @return `true` if request is valid, `false` otherwise.
   */
  bool checkJsonRPCSpec(const json& request);

  /**
   * Helper function to get the method of the JSON-RPC request.
   * @param request The request object.
   * @return The method inside the request, or `invalid` if the method is not found.
   */
  Methods getMethod(const json& request);

  /**
   * Check if `web3_clientVersion` is valid.
   * @param request The request object.
   */
  void web3_clientVersion(const json& request);

  /**
   * Get the string (in bytes) to hash.
   * @param request The request object.
   * @return The bytes to be hashed
   */
  Bytes web3_sha3(const json& request);

  /**
  * Check if `net_version` is valid.
  * @param request The request object.
  */
  void net_version(const json& request);

  /**
  * Check if `net_listening` is valid.
   * @param request The request object.
  */
  void net_listening(const json& request);

  /**
  * Check if `net_peerCount` is valid.
  * @param request The request object.
  */
  void net_peerCount(const json& request);

  /**
  * Check if `eth_protocolVersion` is valid.
  * @param request The request object.
  */
  void eth_protocolVersion(const json& request);

  /**
   * Get the block hash of a `eth_getBlockByHash` request.
   * @param request The request object.
   * @return A pair of block hash and bool (include transactions).
   */
  std::pair<Hash,bool> eth_getBlockByHash(const json& request);

  /**
   * Get the block height of a `eth_getBlockByNumber` request.
   * @param request The request object.
   * @param storage Pointer to the blockchain's storage (in case of "latest" or "pending" block).
   * @return A pair of block height and bool (include transactions).
   */
  std::pair<uint64_t,bool> eth_getBlockByNumber(
    const json& request, const std::unique_ptr<Storage>& storage
  );

  /**
   * Get the block hash of a `eth_getBlockTransactionCountByHash` request.
   * @param request The request object.
   * @return The block hash.
   */
   Hash eth_getBlockTransactionCountByHash(const json& request);

  /**
   * Get the block number of a `eth_getBlockTransactionCountByNumber` request.
   * @param request The request object.
   * @param storage Pointer to the blockchain's storage (in case of "latest" or "pending" block).
   * @return The block number.
   */
  uint64_t eth_getBlockTransactionCountByNumber(
    const json& request, const std::unique_ptr<Storage>& storage
  );

  /**
   * Check if `eth_chainId` is valid.
   * @param request The request object.
   */
  void eth_chainId(const json& request);

  /**
   * Check if `eth_syncing` is valid.
   * @param request The request object.
   */
  void eth_syncing(const json& request);

  /**
   * Check if `eth_coinbase` is valid.
   * @param request The request object.
   */
  void eth_coinbase(const json& request);

  /**
   * Check if eth_blockNumber is valid.
   * @param request The request object.
   */
  void eth_blockNumber(const json& request);

  /**
   * Check and parse a given `eth_call` request.
   * @param request The request object.
   * @param storage Pointer to the blockchain's storage.
   * @return A tuple with the call response data (from, to, gas, gasPrice, value, functor, data).
   */
  ethCallInfoAllocated eth_call(const json& request, const std::unique_ptr<Storage>& storage);

  /**
   * Check and parse a given `eth_estimateGas` request.
   * @param request The request object.
   * @param storage Reference pointer to the blockchain's storage.
   * @return A tuple with the call response data (from, to, gas, gasPrice, value, functor, data).
   */
  ethCallInfoAllocated eth_estimateGas(const json& request, const std::unique_ptr<Storage>& storage);

  /**
   * Check if `eth_gasPrice` is valid.
   * @param request The request object.
   */
  void eth_gasPrice(const json& request);

  /**
   * Parse an `eth_getLogs` call's parameters.
   * @param request The request object.
   * @param storage Reference pointer to the blockchain's storage.
   * @return A tuple with starting and ending block height, address and a list of topics.
   */
  std::tuple<uint64_t, uint64_t, Address, std::vector<Hash>> eth_getLogs(
    const json& request, const std::unique_ptr<Storage>& storage
  );

  /**
   * Parse an `eth_getBalance` address and check if it is valid.
   * @param request The request object.
   * @param storage Pointer to the blockchain's storage.
   * @return The requested address.
   */
  Address eth_getBalance(const json& request, const std::unique_ptr<Storage>& storage);

  /**
   * Parse an `eth_getTransactionCount` address and check if it is valid.
   * @param request The request object.
   * @param storage Pointer to the blockchain's storage.
   * @return The requested address.
   */
  Address eth_getTransactionCount(const json& request, const std::unique_ptr<Storage>& storage);

  /**
   * Parse an `eth_getCode` address and check if it is valid.
   * @param request The request object.
   * @param storage Pointer to the blockchain's storage.
   * @return The requested address.
   */
  Address eth_getCode(const json& request, const std::unique_ptr<Storage>& storage);

  /**
   * Parse a `eth_sendRawTransaction` tx and check if it is valid.
   * @param request The request object.
   * @param requiredChainId The chain ID that the transaction will be sent to.
   * @return The built transaction object.
   */
  TxBlock eth_sendRawTransaction(const json& request, const uint64_t& requiredChainId);

  /**
   * Parse a `eth_getTransactionByHash` transaction hash and check if it is valid.
   * @param request The request object.
   * @return The built transaction hash object.
   */
  Hash eth_getTransactionByHash(const json& request);

  /**
   * Parse a `eth_getTransactionByBlockHashAndIndex` request and check if it is valid.
   * @param request The request object.
   * @return A pair of block hash and index.
   */
  std::pair<Hash,uint64_t> eth_getTransactionByBlockHashAndIndex(const json& request);

  /**
   * Parse a `eth_getTransactionByBlockNumberAndIndex` request and check if it is valid.
   * @param request The request object.
   * @param storage Reference pointer to the blockchain's storage.
   * @return A pair of block height and index.
   */
  std::pair<uint64_t,uint64_t> eth_getTransactionByBlockNumberAndIndex(
    const json& request, const std::unique_ptr<Storage>& storage
  );

  /**
   * Parse a `eth_getTransactionReceipt` request and check if it is valid.
   * @param request The request object.
   * @return The build transaction hash object.
   */
  Hash eth_getTransactionReceipt(const json& request);
}

#endif /// JSONRPC_DECODING_H
