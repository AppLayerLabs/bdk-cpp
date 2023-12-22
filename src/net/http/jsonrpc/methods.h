/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_METHODS_H
#define JSONRPC_METHODS_H

#include <unordered_map>
#include <string>

/**
 * Namespace for Ethereum's JSON-RPC Specification.
 *
 * See the following links for more info:
 *
 * https://ethereum.org/pt/developers/docs/apis/json-rpc/ (Most updated)
 *
 * https://ethereum.github.io/execution-apis/api-documentation/ (Has regex for the methods)
 *
 * https://eips.ethereum.org/EIPS/eip-1474#error-codes (Respective error codes)
 */
namespace JsonRPC {
  /**
   * Enum with all known methods for Ethereum's JSON-RPC.
   *
   * Check the following list for reference (`COMMAND === IMPLEMENTATION STATUS`):
   *
   * ```
   * invalid =================================== N/A
   * web3_clientVersion ======================== DONE (RETURNS APPCHAIN SYSTEM SPECIFIC VERSION)
   * web3_sha3 ================================= DONE
   * net_version =============================== DONE (RETURNS APPCHAIN VERSION)
   * net_listening ============================= TODO: WAITING FOR BLOCKCHAIN
   * net_peerCount ============================= DONE
   * eth_protocolVersion ======================= DONE (RETURNS ORBITERSDK VERSION)
   * eth_getBlockByHash ======================== DONE
   * eth_getBlockByNumber ====================== DONE
   * eth_getBlockTransactionCountByHash ======== DONE
   * eth_getBlockTransactionCountByNumber ====== DONE
   * eth_getUncleCountByBlockHash ============== CAN'T IMPLEMENT, WE ARE NOT DAG (Directed Acyclic Graph)
   * eth_getUncleCountByBlockNumber ============ CAN'T IMPLEMENT, WE ARE NOT DAG (Directed Acyclic Graph)
   * eth_chainId =============================== DONE
   * eth_syncing =============================== TODO: WAITING FOR BLOCKCHAIN
   * eth_coinbase ============================== TODO: WAITING FOR OPTIONS
   * eth_accounts ============================== NOT IMPLEMENTED: NODE IS NOT A WALLET
   * eth_blockNumber =========================== DONE
   * eth_call ================================== DONE
   * eth_estimateGas =========================== DONE
   * eth_createAccessList ====================== NOT IMPLEMENTED: NOT SUPPORTED BY THE BLOCKCHAIN, WE ARE NOT AN EVM
   * eth_gasPrice ============================== DONE
   * eth_maxPriorityFeePerGas ================== TODO: IMPLEMENT THIS
   * eth_feeHistory ============================ TODO: IMPLEMENT THIS, SEE https://docs.alchemy.com/reference/eth-feehistory
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
   * eth_getCode =============================== HALF DONE: ALWAYS ANSWER WITH "0x" AS WE DON'T STORE ANY BYTECODE
   * eth_getProof ============================== NOT IMPLEMENTED: WE DON'T HAVE MERKLE PROOFS FOR ACCOUNTS, ONLY FOR TXS
   * eth_sendTransaction ======================= NOT IMPLEMENTED: NODE IS NOT A WALLET
   * eth_sendRawTransaction ==================== DONE
   * eth_getRawTransaction ===================== DONE
   * eth_getTransactionByHash ================== DONE
   * eth_getTransactionByBlockHashAndIndex ===== DONE
   * eth_getTransactionByBlockNumberAndIndex === DONE
   * eth_getTransactionReceipt ================= DONE
   * ```
   */
  enum Methods {
    invalid,
    web3_clientVersion,
    web3_sha3,
    net_version,
    net_listening,
    net_peerCount,
    eth_protocolVersion,
    eth_getBlockByHash,
    eth_getBlockByNumber,
    eth_getBlockTransactionCountByHash,
    eth_getBlockTransactionCountByNumber,
    eth_getUncleCountByBlockHash,
    eth_getUncleCountByBlockNumber,
    eth_chainId,
    eth_syncing,
    eth_coinbase,
    eth_accounts,
    eth_blockNumber,
    eth_call,
    eth_estimateGas,
    eth_createAccessList,
    eth_gasPrice,
    eth_maxPriorityFeePerGas,
    eth_feeHistory,
    eth_newFilter,
    eth_newBlockFilter,
    eth_newPendingTransactionFilter,
    eth_uninstallFilter,
    eth_getFilterChanges,
    eth_getFilterLogs,
    eth_getLogs,
    eth_mining,
    eth_hashrate,
    eth_getWork,
    eth_submitWork,
    eth_submitHashrate,
    eth_sign,
    eth_signTransaction,
    eth_getBalance,
    eth_getStorageAt,
    eth_getTransactionCount,
    eth_getCode,
    eth_getProof,
    eth_sendTransaction,
    eth_sendRawTransaction,
    eth_getRawTransaction,
    eth_getTransactionByHash,
    eth_getTransactionByBlockHashAndIndex,
    eth_getTransactionByBlockNumberAndIndex,
    eth_getTransactionReceipt
  };

  /// Lookup table for the implemented methods.
  inline extern const std::unordered_map<std::string, Methods> methodsLookupTable = {
    { "web3_clientVersion", web3_clientVersion },
    { "web3_sha3", web3_sha3 },
    { "net_version", net_version },
    { "net_listening", net_listening },
    { "net_peerCount", net_peerCount },
    { "eth_protocolVersion", eth_protocolVersion },
    { "eth_getBlockByHash", eth_getBlockByHash },
    { "eth_getBlockByNumber", eth_getBlockByNumber },
    { "eth_getBlockTransactionCountByHash", eth_getBlockTransactionCountByHash },
    { "eth_getBlockTransactionCountByNumber", eth_getBlockTransactionCountByNumber },
    { "eth_chainId", eth_chainId },
    { "eth_syncing", eth_syncing },
    { "eth_coinbase", eth_coinbase },
    { "eth_blockNumber", eth_blockNumber },
    { "eth_call", eth_call },
    { "eth_estimateGas", eth_estimateGas },
    { "eth_gasPrice", eth_gasPrice },
    { "eth_getBalance", eth_getBalance },
    { "eth_getTransactionCount", eth_getTransactionCount },
    { "eth_getCode", eth_getCode },
    { "eth_sendTransaction", eth_sendTransaction },
    { "eth_sendRawTransaction", eth_sendRawTransaction },
    { "eth_getTransactionByHash", eth_getTransactionByHash },
    { "eth_getTransactionByBlockHashAndIndex", eth_getTransactionByBlockHashAndIndex },
    { "eth_getTransactionByBlockNumberAndIndex", eth_getTransactionByBlockNumberAndIndex },
    { "eth_getTransactionReceipt", eth_getTransactionReceipt }
  };
}

#endif // JSONRPC_METHODS_H
