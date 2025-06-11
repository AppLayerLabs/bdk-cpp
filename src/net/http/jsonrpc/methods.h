/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_METHODS_H
#define JSONRPC_METHODS_H

#include "../../../libs/json.hpp" // Makes no difference but it should be here regardless

#include "../../../core/state.h" // dump.h -> utils/db.h, storage.h

#include "../../p2p/managernormal.h"

#include "error.h"

/**
 * Namespace with all known methods for Ethereum's JSON-RPC.
 *
 * Check the following list for reference (`COMMAND === IMPLEMENTATION STATUS`):
 *
 * ```
 * invalid =================================== N/A
 * web3_clientVersion ======================== DONE (RETURNS APPCHAIN SYSTEM SPECIFIC VERSION)
 * web3_sha3 ================================= DONE
 * net_version =============================== DONE (RETURNS APPCHAIN VERSION)
 * net_listening ============================= DONE (hardcoded to true)
 * net_peerCount ============================= DONE
 * eth_protocolVersion ======================= DONE (RETURNS BDK VERSION)
 * eth_getBlockByHash ======================== DONE
 * eth_getBlockByNumber ====================== DONE
 * eth_getBlockTransactionCountByHash ======== DONE
 * eth_getBlockTransactionCountByNumber ====== DONE
 * eth_getUncleCountByBlockHash ============== CAN'T IMPLEMENT, WE ARE NOT DAG (Directed Acyclic Graph)
 * eth_getUncleCountByBlockNumber ============ CAN'T IMPLEMENT, WE ARE NOT DAG (Directed Acyclic Graph)
 * eth_chainId =============================== DONE
 * eth_syncing =============================== DONE (hardcoded to false)
 * eth_coinbase ============================== DONE
 * eth_accounts ============================== NOT IMPLEMENTED: NODE IS NOT A WALLET
 * eth_blockNumber =========================== DONE
 * eth_call ================================== DONE
 * eth_estimateGas =========================== DONE
 * eth_createAccessList ====================== NOT IMPLEMENTED: NOT SUPPORTED BY THE BLOCKCHAIN, WE ARE NOT AN EVM
 * eth_gasPrice ============================== DONE
 * eth_maxPriorityFeePerGas ================== TODO: IMPLEMENT THIS
 * eth_feeHistory ============================ DONE - see https://docs.alchemy.com/reference/eth-feehistory
 * eth_newFilter ============================= NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
 * eth_newBlockFilter ======================== NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
 * eth_newPendingTransactionFilter =========== NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
 * eth_uninstallFilter ======================= NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
 * eth_getFilterChanges ====================== NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
 * eth_getFilterLogs ========================= NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
 * eth_getLogs =============================== DONE
 * eth_mining ================================ NOT IMPLEMENTED: WE ARE RDPOS NOT POW
 * eth_hashrate ============================== NOT IMPLEMENTED: WE ARE RDPOS NOT POW
 * eth_getWork =============================== NOT IMPLEMENTED: WE ARE RDPOS NOT POW
 * eth_submitWork ============================ NOT IMPLEMENTED: WE ARE RDPOS NOT POW
 * eth_submitHashrate ======================== NOT IMPLEMENTED: WE ARE RDPOS NOT POW
 * eth_sign ================================== NOT IMPLEMENTED: NODE IS NOT A WALLET
 * eth_signTransaction ======================= NOT IMPLEMENTED: NODE IS NOT A WALLET
 * eth_getBalance ============================ DONE
 * eth_getStorageAt ========================== NOT IMPLEMENTED: WE DON'T SUPPORT STORAGE BECAUSE WE ARE NOT AN EVM
 * eth_getTransactionCount =================== DONE
 * eth_getCode =============================== DONE
 * eth_getProof ============================== NOT IMPLEMENTED: WE DON'T HAVE MERKLE PROOFS FOR ACCOUNTS, ONLY FOR TXS
 * eth_sendTransaction ======================= NOT IMPLEMENTED: NODE IS NOT A WALLET
 * eth_sendRawTransaction ==================== DONE
 * eth_getRawTransaction ===================== DONE
 * eth_getTransactionByHash ================== DONE
 * eth_getTransactionByBlockHashAndIndex ===== DONE
 * eth_getTransactionByBlockNumberAndIndex === DONE
 * eth_getTransactionReceipt ================= DONE
 * eth_maxPriorityFeePerGas ================== DONE
 * ```
 */
namespace jsonrpc {
  /**
   * Helper function for getting a block's number.
   * @param storage Reference to the blockchain's storage.
   * @param hash The block's hash.
   * @return The block's number, or std::nullopt if the block does not exist.
   */
  static std::optional<uint64_t> getBlockNumber(const Storage& storage, const Hash& hash) {
    if (const auto block = storage.getBlock(hash); block != nullptr) return block->getNHeight();
    return std::nullopt;
  }

  /**
   * Helper templated function for creating a vector from a given range.
   * @tparam T The data type.
   * @tparam R The range type.
   * @param range The range to convert.
   * @return The built vector.
   */
  template<typename T, std::ranges::input_range R>
  requires std::convertible_to<std::ranges::range_value_t<R>, T>
  static std::vector<T> makeVector(R&& range) {
    std::vector<T> res(std::ranges::size(range));
    std::ranges::copy(std::forward<R>(range), res.begin());
    return res;
  }

  /**
   * Helper function for forbidding the usage of the "params" field in a JSON object
   * in methods where it is not required.
   * @param request The JSON object to check.
   * @throw DynamicException if the field exists and is not empty.
   */
  static inline void forbidParams(const json& request) {
    if (request.contains("params") && !request["params"].empty()) {
      throw DynamicException("\"params\" are not required for method");
    }
  }

  /**
   * Helper function for forbidding the call of a method that requires indexing
   * in a node that has it disabled.
   * @param storage Reference to the blockchain's storage.
   * @param method The method's name.
   * @throw Error if indexing is disabled.
   */
  static inline void requiresIndexing(const Storage& storage, std::string_view method) {
    if (storage.getIndexingMode() == IndexingMode::DISABLED) {
      throw Error::methodNotAvailable(method);
    }
  }

  /**
   * Helper function for forbidding the call of a method that requires debug indexing
   * in a node that has it disabled or set to something other than RPC_TRACE.
   * @param storage Reference to the blockchain's storage.
   * @param method The method's name.
   * @throw Error if debug indexing is disabled.
   */
  static inline void requiresDebugIndexing(const Storage& storage, std::string_view method) {
    if (storage.getIndexingMode() != IndexingMode::RPC_TRACE) {
      throw Error::methodNotAvailable(method);
    }
  }

  /**
   * Helper function for getting a block serialized to JSON format.
   * @param block Pointer to the block.
   * @param includeTransactions If `true`, includes the block's transactions. If `false`, include only their hashes.
   * @return The block as a JSON object.
   */
  json getBlockJson(const FinalizedBlock* block, bool includeTransactions);

  /**
   * Helper function for parsing an EVMC message.
   * @param request The JSON request object.
   * @param storage Reference to the blockchain's storage.
   * @param recipientRequired If `true`, forcibly parses the "to" address.
   *                          If `false`, parse an empty address if the field does not exist in the request.
   * @return A pair consisting of the message and its parsed result as a raw bytes string.
   * @throw Error if block number is not latest.
   */
  std::pair<Bytes, evmc_message> parseEvmcMessage(const json& request, const Storage& storage, bool recipientRequired);

  // ========================================================================
  //  METHODS START HERE
  // ========================================================================

  ///@{
  /** Execute the respective RPC method. */
  json web3_clientVersion(const json& request, const Options& options);
  json web3_sha3(const json& request);
  json net_version(const json& request, const Options& options);
  json net_listening(const json& request);
  json eth_protocolVersion(const json& request, const Options& options);
  json net_peerCount(const json& request, const P2P::ManagerNormal& manager);
  json eth_getBlockByHash(const json& request, const Storage& storage);
  json eth_getBlockByNumber(const json& request, const Storage& storage);
  json eth_getBlockTransactionCountByHash(const json& request, const Storage& storage);
  json eth_getBlockTransactionCountByNumber(const json& request, const Storage& storage);
  json eth_chainId(const json& request, const Options& options);
  json eth_syncing(const json& request);
  json eth_coinbase(const json& request, const Options& options);
  json eth_blockNumber(const json& request, const Storage& storage);
  json eth_call(const json& request, const Storage& storage, State& state);
  json eth_estimateGas(const json& request, const Storage& storage, State& state);
  json eth_gasPrice(const json& request);
  json eth_feeHistory(const json& request, const Storage& storage);
  json eth_getLogs(const json& request, const Storage& storage, const Options& options);
  json eth_getBalance(const json& request, const Storage& storage, const State& state);
  json eth_getTransactionCount(const json& request, const Storage& storage, const State& state);
  json eth_getCode(const json& request, const Storage& storage, const State& state);
  json eth_sendRawTransaction(const json& request, uint64_t chainId, State& state, P2P::ManagerNormal& p2p);
  json eth_getTransactionByHash(const json& request, const Storage& storage, const State& state);
  json eth_getTransactionByBlockHashAndIndex(const json& request, const Storage& storage);
  json eth_getTransactionByBlockNumberAndIndex(const json& request, const Storage& storage);
  json eth_getTransactionReceipt(const json& request, const Storage& storage, const Options& options);
  json eth_getUncleByBlockHashAndIndex();
  json eth_maxPriorityFeePerGas(const json& request, const Options& options);
  json txpool_content(const json& request, const State& state);
  json debug_traceBlockByNumber(const json& request, const Storage& storage);
  json debug_traceTransaction(const json& request, const Storage& storage);
  json appl_dumpState(const json& request, State& state, const Options& options);
  ///@}
} // namespace jsonrpc

#endif // JSONRPC_METHODS_H
