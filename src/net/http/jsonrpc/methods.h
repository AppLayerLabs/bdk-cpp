#ifndef JSONRPC_METHODS_H
#define JSONRPC_METHODS_H
#include <unordered_map>
#include <string>

/// Ethereum JSON-RPC Specification
/// https://ethereum.org/pt/developers/docs/apis/json-rpc/ <- Most updated.
/// https://ethereum.github.io/execution-apis/api-documentation/ <- Has regex for the methods.
/// https://eips.ethereum.org/EIPS/eip-1474#error-codes <- Respective error codes

/// COMMAND ----------------------------------------------- IMPLEMENTATION STATUS
namespace JsonRPC {
  enum Methods {
    invalid,
    web3_clientVersion,                                     /// DONE (RETURNS APPCHAIN SYSTEM SPECIFIC VERSION)
    web3_sha3,                                              /// DONE
    net_version,                                            /// DONE (RETURNS APPCHAIN VERSION)
    net_listening,                                          /// TODO: WAITING FOR BLOCKCHAIN
    net_peerCount,                                          /// DONE
    eth_protocolVersion,                                    /// DONE (RETURNS ORBITERSDK VERSION)
    eth_getBlockByHash,                                     /// DONE
    eth_getBlockByNumber,                                   /// DONE
    eth_getBlockTransactionCountByHash,                     /// DONE
    eth_getBlockTransactionCountByNumber,                   /// DONE
    eth_getUncleCountByBlockHash,                           /// CAN'T IMPLEMENT, WE ARE NOT DAG (DAG = Directed Acyclic Graph)
    eth_getUncleCountByBlockNumber,                         /// CAN'T IMPLEMENT, WE ARE NOT DAG (DAG = Directed Acyclic Graph)
    eth_chainId,                                            /// DONE.
    eth_syncing,                                            /// TODO: WAITING FOR BLOCKCHAIN
    eth_coinbase,                                           /// TODO: WAITING FOR OPTIONS
    eth_accounts,                                           /// NOT IMPLEMENTED: NODE IS NOT A WALLET
    eth_blockNumber,                                        /// DONE
    eth_call,                                               /// TODO: WAITING FOR CONTRACTS
    eth_estimateGas,                                        /// DONE
    eth_createAccessList,                                   /// NOT IMPLEMENTED: NOT SUPPORTED BY THE BLOCKCHAIN, WE ARE NOT An EVM
    eth_gasPrice,                                           /// DONE
    eth_maxPriorityFeePerGas,                               /// NOT IMPLEMENTED: WE DON'T SUPPORT EIP-1559 TXS
    eth_newFilter,                                          /// NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
    eth_newBlockFilter,                                     /// NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
    eth_newPendingTransactionFilter,                        /// NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
    eth_uninstallFilter,                                    /// NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
    eth_getFilterChanges,                                   /// NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
    eth_getFilterLogs,                                      /// NOT IMPLEMENTED: WE DON'T SUPPORT FILTERS (FOR NOW)
    eth_getLogs,                                            /// NOT IMPLEMENTED: LOGS ARE LOCATED AT DEBUG.LOG
    eth_mining,                                             /// NOT IMPLEMENTED: WE ARE RDPOS NOT POW
    eth_hashrate,                                           /// NOT IMPLEMENTED: WE ARE RDPOS NOT POW
    eth_getWork,                                            /// NOT IMPLEMENTED: WE ARE RDPOS NOT POW
    eth_submitWork,                                         /// NOT IMPLEMENTED: WE ARE RDPOS NOT POW
    eth_submitHashrate,                                     /// NOT IMPLEMENTED: WE ARE RDPOS NOT POW
    eth_sign,                                               /// NOT IMPLEMENTED: NODE IS NOT A WALLET
    eth_signTransaction,                                    /// NOT IMPLEMENTED: NODE IS NOT A WALLET
    eth_getBalance,                                         /// DONE
    eth_getStorageAt,                                       /// NOT IMPLEMENTED: WE DON'T SUPPORT STORAGED/WE ARE NOT EVM
    eth_getTransactionCount,                                /// DONE
    eth_getCode,                                            /// HALF DONE: ALWAYS ANSWER WITH 0x
    eth_getProof,                                           /// NOT IMPLEMENTED: WE DON'T HAVE MERKLE PROOFS FOR ACCOUNTS, ONLY FOR TXS
    eth_sendTransaction,                                    /// NOT IMPLEMENTED: NODE ARE NOT A WALLET
    eth_sendRawTransaction,                                 /// HALF DONE, WAITING FOR BLOCKCHAIN TO PROPERLY BROADCAST TX'S TO OTHER NODES.
    eth_getRawTransaction,
    eth_getTransactionByHash,                               /// DONE
    eth_getTransactionByBlockHashAndIndex,                  /// DONE
    eth_getTransactionByBlockNumberAndIndex,                /// DONE
    eth_getTransactionReceipt,                              /// DONE
  };

  inline extern const std::unordered_map<std::string, Methods> methodsLookupTable = {
    {"eth_getBlockByHash", eth_getBlockByHash },
    { "web3_clientVersion", web3_clientVersion },
    { "web3_sha3", web3_sha3 },
    { "net_version", net_version },
    { "net_listening", net_listening },
    { "net_peerCount", net_peerCount },
    { "eth_protocolVersion", eth_protocolVersion },
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