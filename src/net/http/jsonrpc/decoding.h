#ifndef JSONRPC_DECODING_H
#define JSONRPC_DECODING_H

#include <regex>

#include "../../../utils/utils.h"
#include "../../../utils/strings.h"
#include "../../../utils/tx.h"

#include "methods.h"

/// Forward declaration.
class Storage;

namespace JsonRPC {
  namespace Decoding {

    /**
     * Check if caller is a valid JSON-RPC Request.
     * does not check if the method is valid, only if follows JSON-RPC 2.0 spec.
     */
    bool checkJsonRPCSpec(const json& request);

    /**
     * Get the method of the JSON-RPC Request.
     * @param json The JSON-RPC Request.
     * @return The method of the JSON-RPC Request.
     * @return Method::invalid if the method is not found.
     */
    Methods getMethod(const json& request);

    /**
     * Check if web3_clientVersion is valid.
     * @param request
     * Throws if invalid
     */
    void web3_clientVersion(const json& request);

    /**
     * Get the string (in bytes) to hash
     * @param request
     * @return String to hash
     * Throws if invalid
     */
    std::string web3_sha3(const json& request);

    /**
    * Check if net_version is valid.
    * @param request
    * Throws if invalid
    */
    void net_version(const json& request);

    /**
    * Check if net_listening is valid.
    * @param request
    * Throws if invalid
    */
    void net_listening(const json& request);

    /**
    * Check if net_peerCount is valid.
    * @param request
    * Throws if invalid
    */
    void net_peerCount(const json& request);

    /**
    * Check if eth_protocolVersion is valid.
    * @param request
    * Throws if invalid
    */
    void eth_protocolVersion(const json& request);

    /**
     * Get the block Hash of a eth_getBlockByHash request
     * @param request
     * @return std::pair<Hash,bool> Hash = Block hash, Bool = include transactions.
     * throws if invalid.
     */
    std::pair<Hash,bool> eth_getBlockByHash(const json& request);

    /** Get the number of the block of a eth_getBlockByNumber request
     * Pointer to storage is needed in case of "latest" or "pending" block.
     * @param request
     * @return std::pair<Hash,uint64_t> Hash = block hash, Bool = include transactions.
     * throws if invalid.
     */
    std::pair<uint64_t,bool> eth_getBlockByNumber(const json& request, const std::unique_ptr<Storage> &storage);

    /**
     * Get the block hash for of a eth_getBlockTransactionCountByHash request
     * @param request
     * @return Hash = Block hash.
     * throws if invalid.
     */
     Hash eth_getBlockTransactionCountByHash(const json& request);

    /**
     * Get the block number for of a eth_getBlockTransactionCountByNumber request
     * Pointer to storage is needed in case of "latest" or "pending" block.
     * @param request
     * @return uint64_t = Block number.
     * throws if invalid.
     */
    uint64_t eth_getBlockTransactionCountByNumber(const json& request, const std::unique_ptr<Storage> &storage);

    /** Check if eth_chainId is valid.
     * @param request
     * Throws if invalid.
     */
    void eth_chainId(const json& request);

    /** Check if eth_syncing is valid
     * @param request
     * Throws if invalid.
     */
    void eth_syncing(const json& request);

    /** Check if eth_coinbase is valid
     * @param request
     * Throws if invalid
     */
    void eth_coinbase(const json& request);

    /** Check if eth_blockNumber is valid
     * @param request
     * Throws if invalid
     */
    void eth_blockNumber(const json& request);

    /** Check and parse a given eth_call request
     * @param request
     * @return std::tuple<Address,Address,uint64_t, uint256_t, uint256_t, std::string> = (from, to, gas, gasPrice, value, data)
     */
    std::tuple<Address,Address,uint64_t, uint256_t, uint256_t, std::string> eth_call(const json& request);

    /** Check and parse a given eth_estimateGas request
     * @param request
     * @return std::tuple<Address,Address,uint64_t, uint256_t, uint256_t, std::string> = (from, to, gas, gasPrice, value, data)
     */
    std::tuple<Address,Address,uint64_t, uint256_t, uint256_t, std::string> eth_estimateGas(const json& request);

    /** Check if eth_gasPrice is valid
     * @param request
     * Throws if invalid
     */
    void eth_gasPrice(const json& request);

    /** Parse eth_getBalance Address, check if valid.
     * @param request
     * @return Address
     * Throws if invalid
     */
    Address eth_getBalance(const json& request);

    /** Parse eth_getTransactionCount Address, check if valid
     * @param request
     * @return Address
     * Throws if invalid
     */
    Address eth_getTransactionCount(const json& request);

    /** Parse eth_sendRawTransaction Tx, check if valid
     * @param request
     * @return TxBlock
     * Throws if invalid
     */
    TxBlock eth_sendRawTransaction(const json& request);

    /** Parse a eth_getTransactionByHash tx Hash
     * @param request
     * @return Hash
     * Throws if invalid
     */
    Hash eth_getTransactionByHash(const json& request);

    /** Parse a eth_getTransactionByBlockHashAndIndex request
     * @param request
     * @return std::pair<Hash,uint64_t> = (blockHash, index)
     * Throws if invalid
     */
    std::pair<Hash,uint64_t> eth_getTransactionByBlockHashAndIndex(const json& request);

    /** Parse a eth_getTransactionByBlockNumberAndIndex request
     * @param request
     * @return std::pair<uint64_t,uint64_t> = (blockNumber, index)
     * Throws if invalid
     */
    std::pair<uint64_t,uint64_t> eth_getTransactionByBlockNumberAndIndex(const json& request, const std::unique_ptr<Storage> &storage);

    /**
     * Parse a eth_getTransactionReceipt request
     * @param request
     * @return Hash = txHash
     * Throws if invalid
     */
    Hash eth_getTransactionReceipt(const json& request);
  }
}






#endif /// JSONRPC_DECODING_H