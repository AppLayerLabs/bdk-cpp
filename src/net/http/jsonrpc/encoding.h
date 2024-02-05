/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_ENCODING_H
#define JSONRPC_ENCODING_H

#include "../../../utils/utils.h"
#include "../../../utils/block.h"
#include "../../../utils/tx.h"
#include "../../../utils/options.h"
#include "../../p2p/managernormal.h"

// Forward declarations.
namespace P2P { class ManagerNormal; }
class Storage;
class State;

/// Namespace for encoding JSON-RPC data.
namespace JsonRPC::Encoding {
  /**
   * Helper function to get a block in JSON format.
   * Used with functions related to getting blocks.
   * We can use a reference to a shared_ptr because the functions calling it
   * will first create a copy from Storage, then pass it to this function.
   * @param block The block to use as reference for building the JSON response.
   * @param includeTransactions If `true`, includes the block's transactions in the JSON response.
   * @return The block's contents as a JSON object.
   */
  json getBlockJson(const std::shared_ptr<const Block>& block, bool includeTransactions);

  /**
   * Encode a `web3_clientVersion` response.
   * @param options Pointer to the options singleton.
   * @return The encoded JSON response.
   */
  json web3_clientVersion(const std::unique_ptr<Options>& options);

  /**
   * Encode a `web3_sha3` response.
   * @param data The string to be hashed.
   * @return The encoded JSON response.
   */
  json web3_sha3(const BytesArrView data);

  /**
   * Encode a `net_version` response.
   * @param options Pointer to the options singleton.
   * @return The encoded JSON response.
   */
  json net_version(const std::unique_ptr<Options>& options);

  /**
   * Encode a `net_listening` response.
   * @return The encoded JSON response.
   */
  // TODO: WAITING FOR BLOCKCHAIN
  json net_listening();

  /**
   * Encode a `net_peerCount` response.
   * @param manager Pointer to the P2P connection manager.
   * @return The encoded JSON response.
   */
  json net_peerCount(const std::unique_ptr<P2P::ManagerNormal>& manager);

  /**
   * Encode a `eth_protocolVersion` response.
   * @param options Pointer to the options singleton.
   * @return The encoded JSON response.
   */
  json eth_protocolVersion(const std::unique_ptr<Options>& options);

  /**
   * Encode a `eth_getBlockByHash` response.
   * @param blockInfo A pair of block hash and boolean (include transactions).
   * @param storage Pointer to the blockchain's storage.
   * @return The encoded JSON response.
   */
  json eth_getBlockByHash(
    const std::pair<Hash,bool>& blockInfo, const std::unique_ptr<Storage>& storage
  );

  /**
   * Encode a `eth_getBlockByNumber` response.
   * @param blockInfo A pair of block number and boolean (include transactions).
   * @param storage Pointer to the blockchain's storage.
   * @return The encoded JSON response.
   */
  json eth_getBlockByNumber(
    const std::pair<uint64_t,bool>& blockInfo, const std::unique_ptr<Storage>& storage
  );

  /**
   * Encode a `eth_getBlockTransactionCountByHash` response.
   * @param blockHash The block to count transactions from.
   * @param storage Pointer to the blockchain's storage.
   * @return The encoded JSON response.
   */
  json eth_getBlockTransactionCountByHash(
    const Hash& blockHash, const std::unique_ptr<Storage>& storage
  );

  /**
   * Encode a `eth_getBlockTransactionCountByNumber` response.
   * @param blockNumber The block height to count transactions from.
   * @param storage Pointer to the blockchain's storage.
   * @return The encoded JSON response.
   */
  json eth_getBlockTransactionCountByNumber(
    const uint64_t& blockNumber, const std::unique_ptr<Storage>& storage
  );

  /**
   * Encode a `eth_chainId` response.
   * @param options Pointer to the options singleton.
   * @return The encoded JSON response.
   */
  json eth_chainId(const std::unique_ptr<Options>& options);

  /**
   * Encode a `eth_syncing` response.
   * @return The encoded JSON response.
   */
  // TODO: WAITING FOR BLOCKCHAIN
  json eth_syncing();

  /**
   * Encode a `eth_coinbase` response.
   * @param options Pointer to the options singleton.
   * @return The encoded JSON response.
   */
  json eth_coinbase(const std::unique_ptr<Options>& options);

  /**
   * Encode a `eth_blockNumber` response.
   * @param storage Pointer to the blockchain's storage.
   * @return The encoded JSON response.
   */
  json eth_blockNumber(const std::unique_ptr<Storage>& storage);

  /**
   * Encode a `eth_call` response.
   * @param callInfo Info about the call (from, to, gas, gasPrice, value, functor, data).
   * @param state Pointer to the blockchain's state.
   * @return The encoded JSON response.
   */
  json eth_call(const ethCallInfoAllocated& callInfo, const std::unique_ptr<State>& state);

  /**,
   * Encode a `eth_estimateGas` response.
   * @param callInfo Info about the call (from, to, gas, gasPrice, value, functor, data).
   * @param state Pointer to the blockchain's state.
   * @return The encoded JSON response.
   */
  // TODO: We don't really estimate gas because we don't have a Gas structure, it is fixed to 21000
  json eth_estimateGas(const ethCallInfoAllocated& callInfo, const std::unique_ptr<State>& state);

  /**
   * Encode a `eth_gasPrice` response.
   * @return The encoded JSON response.
   */
  // TODO: We don't really estimate gas because we don't have a Gas structure, it is fixed to 21000
  json eth_gasPrice();

  /**
   * Encode a `eth_getLogs` response.
   * @param info A tuple of starting and ending block, address and a list of topics.
   * @param state Reference pointer to blockchain's state.
   * @return The encoded JSON response.
   */
  json eth_getLogs(
    std::tuple<uint64_t, uint64_t, Address, std::vector<Hash>> info,
    const std::unique_ptr<State>& state
  );

  /**
   * Encode a `eth_getBalance` response.
   * @param address The address to get the balance from.
   * @param state Pointer to the blockchain's state.
   * @return The encoded JSON response.
   */
  json eth_getBalance(const Address& address, const std::unique_ptr<State>& state);

  /**
   * Encode a `eth_getTransactionCount` response.
   * @param address The address to get the transaction count from.
   * @param state Pointer to the blockchain's state.
   * @return The encoded JSON response.
   */
  json eth_getTransactionCount(const Address& address, const std::unique_ptr<State>& state);

  /**
   * Encode a `eth_getCode` response (always returns "0x").
   * @param address The address to get the code from.
   * @return The encoded JSON response.
   */
  json eth_getCode(const Address& address);

  /**
   * Encode a `eth_sendRawTransaction` response. Adds the transaction to the mempool.
   * @param tx The transaction to add.
   * @param state Pointer to the blockchain's state.
   * @param p2p Pointer to the P2P connection manager.
   * @return The encoded JSON response.
   */
  // TODO: WAITING FOR BLOCKCHAIN
  json eth_sendRawTransaction(
    const TxBlock& tx, const std::unique_ptr<State>& state, const std::unique_ptr<P2P::ManagerNormal>& p2p
  );

  /**
   * Encode a `eth_getTransactionByHash` response.
   * @param txHash Hash of the transaction to get.
   * @param storage Pointer to the blockchain's storage (if tx is on chain).
   * @param state Pointer to the blockchain's state (if tx is on mempool).
   * @return The encoded JSON response.
   */
  json eth_getTransactionByHash(
    const Hash& txHash, const std::unique_ptr<Storage>& storage, const std::unique_ptr<State>& state
  );

  /**
   * Encode a `eth_getTransactionByBlockHashAndIndex` response.
   * @param requestInfo A pair of block hash and index to look for.
   * @param storage Pointer to the blockchain's storage.
   * @return The encoded JSON response.
   */
  json eth_getTransactionByBlockHashAndIndex(
    const std::pair<Hash,uint64_t>& requestInfo, const std::unique_ptr<Storage>& storage
  );

  /**
   * Encode a `eth_getTransactionByBlockNumberAndIndex` response.
   * @param requestInfo A pair of block height and index to look for.
   * @param storage Pointer to the blockchain's storage.
   * @return The encoded JSON response.
   */
  json eth_getTransactionByBlockNumberAndIndex(
    const std::pair<uint64_t,uint64_t>& requestInfo, const std::unique_ptr<Storage>& storage
  );

  /**
   * Encode a `eth_getTransactionReceipt` response.
   * @param txHash The transaction's hash.
   * @param storage Pointer to the blockchain's storage.
   * @param state Pointer to the blockchain's state.
   * @return The encoded JSON response.
   */
  json eth_getTransactionReceipt(
    const Hash& txHash, const std::unique_ptr<Storage>& storage,
    const std::unique_ptr<State>& state
  );
}

#endif  // JSONRPC_ENCODING_H
