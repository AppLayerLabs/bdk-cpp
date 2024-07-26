#ifndef JSONRPC_METHODS_H
#define JSONRPC_METHODS_H

#include "../../p2p/managernormal.h"

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
 * eth_protocolVersion ======================= DONE (RETURNS BDK VERSION)
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
 * eth_getCode =============================== DONE
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

namespace jsonrpc {

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

json eth_getLogs(const json& request, const Storage& storage, const State& state);

json eth_getBalance(const json& request, const Storage& storage, const State& state);

json eth_getTransactionCount(const json& request, const Storage& storage, const State& state);

json eth_getCode(const json& request, const Storage& storage, const State& state);

json eth_sendRawTransaction(const json& request, uint64_t chainId, State& state, P2P::ManagerNormal& p2p);

json eth_getTransactionByHash(const json& request, const Storage& storage, const State& state);

json eth_getTransactionByBlockHashAndIndex(const json& request, const Storage& storage);

json eth_getTransactionByBlockNumberAndIndex(const json& request, const Storage& storage);

json eth_getTransactionReceipt(const json& request, const Storage& storage, const State& state);

json eth_getUncleByBlockHashAndIndex();

json txpool_content(const json& request, const State& state);

json debug_traceBlockByNumber(const json& request, const Storage& storage);

json debug_traceTransaction(const json& request, const Storage& storage);

} // namespace jsonrpc

#endif // JSONRPC_METHODS_H
