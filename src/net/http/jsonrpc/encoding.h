#ifndef JSONRPC_ENCODING_H
#define JSONRPC_ENCODING_H

#include "../../../utils/utils.h"
#include "../../../utils/block.h"
#include "../../../utils/tx.h"
#include "../../../utils/options.h"

/// Forward Declaration
namespace P2P {
  class ManagerNormal;
}

class Storage;
class State;

namespace JsonRPC {
  namespace Encoding {
    /// HELPER FUNCTIONS
    /**
     * Get a block in JSON format
     * To be used with functions related too getting blocks.
     * We can use a reference to a shared_ptr because the functions calling it
     * will firstly create a copy from Storage, then pass it to this function.
     */
    json getBlockJson(const std::shared_ptr<const Block>& block, bool includeTransactions);

    /// JSON ENCODING FUNCTIONS

    /**
     * JSON Encode a web3_clientVersion response.
     */
    json web3_clientVersion(const std::unique_ptr<Options>& options);

    /**
     * JSON Encode a web3_sha3 request
     * Hashes string argument interpreting as bytes
     */
    json web3_sha3(const std::string& data);

    /**
     * JSON Encode a net_version response
     */
    json net_version(const std::unique_ptr<Options>& options);

    /**
     * JSON Encode a net_listening response
     * TODO: WAITING FOR BLOCKCHAIN
     */
    json net_listening();

    /**
     * JSON Encode a net_peerCount response
     * @params ManagerNormal: pointer to P2P Manager
     */
    json net_peerCount(const std::unique_ptr<P2P::ManagerNormal>& manager);

    /**
     * JSON Encode a eth_protocolVersion response
     * TODO: Wainting for options
     */
    json eth_protocolVersion(const std::unique_ptr<Options>& options);

    /**
     * JSON Encode a eth_getBlockByHash response
     * @params blockInfo: pair of block hash and boolean (include transactions)
     * @params Storage: pointer to storage
     */
    json eth_getBlockByHash(const std::pair<Hash,bool>& blockInfo, const std::unique_ptr<Storage>& storage);

    /**
     * JSON encode a eth_getBlockByNumber response
     * @params blockInfo: pair of block number and boolean (include transactions)
     * @params Storage: pointer to storage
     */
    json eth_getBlockByNumber(const std::pair<uint64_t,bool>& blockInfo, const std::unique_ptr<Storage>& storage);

    /**
     * JSON encode a eth_getBlockTransactionCountByHash responsoe
     * @params Hash: block Hash
     * @params Storage: pointer to storage
     */
    json eth_getBlockTransactionCountByHash(const Hash& blockHash, const std::unique_ptr<Storage>& storage);

    /**
     * JSON encode a eth_getBlockTransactionCountByNumber
     * @params uint64_t block number
     * @params Storage: pointer to storage
     */
    json eth_getBlockTransactionCountByNumber(const uint64_t& blockNumber, const std::unique_ptr<Storage>& storage);

    /**
     * JSON encode a eth_chainId response
     */
    json eth_chainId(const std::unique_ptr<Options>& options);

    /**
     * JSON encode a eth_syncing response
     * TODO: WAITING FOR BLOCKCHAIN
     */
    json eth_syncing();

    /**
     * JSON encode a eth_coinbase response
     */
    json eth_coinbase(const std::unique_ptr<Options>& options);

    /**
     * JSON encode a eth_blockNumber response
     * @params Storage: pointer to storage
     */
    json eth_blockNumber(const std::unique_ptr<Storage>& storage);

    /**
     * JSON encode a eth_call response.
     * @params callInfo: tuple of (from, to, gas, gasPrice, value, data)
     */
    json eth_call(const std::tuple<Address,Address,uint64_t, uint256_t, uint256_t, std::string>& callInfo, const std::unique_ptr<State>& state);

    /**
     * JSON encode a eth_estimateGas response
     * TODO: We don't really estimate gas because we don't have a Gas structure, it is fixed to 21000
     * @params callInfo: tuple of (from, to, gas, gasPrice, value, data)
     */
    json eth_estimateGas(const std::tuple<Address,Address,uint64_t, uint256_t, uint256_t, std::string>& callInfo, const std::unique_ptr<State>& state);


    /**
     * JSON encode a eth_gasPrice resposnoe
     * TODO: We don't really estimate gas because we don't have a Gas structure, it is fixed to 21000
     */
    json eth_gasPrice();

    /**
     * JSON encode a eth_getBalance response
     * @params address: address to get balance from
     * @params State: pointer to state
     */
    json eth_getBalance(const Address& address, const std::unique_ptr<State>& state);

    /**
     * JSON encode a eth_getTransactionCount response
     * @params address: address to get nonce from
     * @params State: pointer to state
     */
    json eth_getTransactionCount(const Address& address, const std::unique_ptr<State>& state);

    /** JSON encode a eth_getCode response
     * @params address: address to get code from
     * returns "0x"
     */
    json eth_getCode(const Address& address);

    /**
     * JSON encode a eth_sendRawTransaction respnse
     * Add tx to mempool
     * @params tx: transaction to add
     * @params State: pointer to state
     * TODO: WAITING FOR BLOCKCHAIN
     */
    json eth_sendRawTransaction(const TxBlock& tx, const std::unique_ptr<State>& state, const std::unique_ptr<P2P::ManagerNormal>& p2p);

    /**
     * JSON encode a eth_getTransactionByHash response
     * @params txHash: hash of transaction to get
     * @params Storage: pointer to storage (Tx is on blockchain)
     * @params State: pointer to state (Tx is on mempool)
     */
    json eth_getTransactionByHash(const Hash& txHash, const std::unique_ptr<Storage>& storage, const std::unique_ptr<State>& state);

    /**
     * JSON encode a eth_getTransactionByBlockHashAndIndex response
     * @params blockHash: given block hash.
     * @params blockIndex: index of tx within block
     * @params Storage: pointer to storage
     */
    json eth_getTransactionByBlockHashAndIndex(const std::pair<Hash,uint64_t>& requestInfo, const std::unique_ptr<Storage>& storage);

    /**
     * JSON encode a eth_getTransactionByBlockNumberAndIndex response
     * @params blockNumber: given block number
     * @params blockIndex: index of tx within block
     * @params Storage: pointer to storage
     */
    json eth_getTransactionByBlockNumberAndIndex(const std::pair<uint64_t,uint64_t>& requestInfo, const std::unique_ptr<Storage>& storage);

    /**
     * JSON encode a eth_getTransactionReceipt response
     * @params txHash: given tx hash
     * @params Storage: pointer to Storage
     */
    json eth_getTransactionReceipt(const Hash& txHash, const std::unique_ptr<Storage>& storage);
  }
}

#endif