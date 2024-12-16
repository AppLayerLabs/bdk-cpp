#ifndef _NODERPCINTERFACE_H_
#define _NODERPCINTERFACE_H_

#include "../libs/json.hpp"

using json = nlohmann::ordered_json;

/**
 * The HTTPServer class is actually used to implement the RPC server for the BDK node,
 * so it takes a reference to a class that implements all the JSON-RPC methods that a
 * node must make available.
 */
class NodeRPCInterface {
public:
  virtual json web3_clientVersion(const json& request) = 0;
  virtual json web3_sha3(const json& request) = 0;
  virtual json net_version(const json& request) = 0;
  virtual json net_listening(const json& request) = 0;
  virtual json eth_protocolVersion(const json& request) = 0;
  virtual json net_peerCount(const json& request) = 0;
  virtual json eth_getBlockByHash(const json& request)= 0;
  virtual json eth_getBlockByNumber(const json& request) = 0;
  virtual json eth_getBlockTransactionCountByHash(const json& request) = 0;
  virtual json eth_getBlockTransactionCountByNumber(const json& request) = 0;
  virtual json eth_chainId(const json& request) = 0;
  virtual json eth_syncing(const json& request) = 0;
  virtual json eth_coinbase(const json& request) = 0;
  virtual json eth_blockNumber(const json& request) = 0;
  virtual json eth_call(const json& request) = 0;
  virtual json eth_estimateGas(const json& request) = 0;
  virtual json eth_gasPrice(const json& request) = 0;
  virtual json eth_feeHistory(const json& request) = 0;
  virtual json eth_getLogs(const json& request) = 0;
  virtual json eth_getBalance(const json& request) = 0;
  virtual json eth_getTransactionCount(const json& request) = 0;
  virtual json eth_getCode(const json& request) = 0;
  virtual json eth_sendRawTransaction(const json& request) = 0;
  virtual json eth_getTransactionByHash(const json& request) = 0;
  virtual json eth_getTransactionByBlockHashAndIndex(const json& request)= 0;
  virtual json eth_getTransactionByBlockNumberAndIndex(const json& request) = 0;
  virtual json eth_getTransactionReceipt(const json& request) = 0;
  virtual json eth_getUncleByBlockHashAndIndex() = 0;
};

#endif
