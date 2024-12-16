/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "blockchain.h"

#include "../utils/logger.h"

Blockchain::Blockchain(const std::string& blockchainPath, std::string instanceId) :
  instanceId_(instanceId),
  options_(Options::fromFile(blockchainPath)),
  comet_(this, instanceId, options_),
  state_(*this), http_(options_.getHttpPort(), *this)
{
}

void Blockchain::start() {
  // Initialize necessary modules
  LOGINFOP("Starting BDK Node...");

  // FIXME/TODO: use cometbft seed-node/PEX to implement discoverynode
  // just setting the relevant config.toml options via Options::cometBFT::config.toml::xxx

  // FIXME/TODO: state saver
  // must checkpoint the entire machine State to disk synchronously (blocking)
  // every X blocks (you can't update the state while you are writing, you must
  // acquire an exclusive lock over the entire State during checkpointing to disk).
  // then, needs a synchronous loadCheckpoint("DB dir/name") function as well.
  // each checkpoint must have its own disk location (instead of writing multiple
  // checkpoints as entries inside the same database files/dir).
  // two ways to do this:
  // - fork the process to duplicate memory then write to disk in the fork
  // - run a dedicated checkpointing node together with the validator node
  // a regular node can just be a checkpointing node itself if it can afford
  // to get a little behind the chain or, if it wants to pay for the memory
  // cost, it can benefit from the process fork checkpointer.

  this->comet_.start();
  this->http_.start();
}

void Blockchain::stop() {
  this->http_.stop();
  this->comet_.stop();
}

json Blockchain::web3_clientVersion(const json& request) { return {}; }
json Blockchain::web3_sha3(const json& request) { return {}; }
json Blockchain::net_version(const json& request) { return {}; }
json Blockchain::net_listening(const json& request) { return {}; }
json Blockchain::eth_protocolVersion(const json& request) { return {}; }
json Blockchain::net_peerCount(const json& request) { return {}; }
json Blockchain::eth_getBlockByHash(const json& request){ return {}; }
json Blockchain::eth_getBlockByNumber(const json& request) { return {}; }
json Blockchain::eth_getBlockTransactionCountByHash(const json& request) { return {}; }
json Blockchain::eth_getBlockTransactionCountByNumber(const json& request) { return {}; }
json Blockchain::eth_chainId(const json& request) { return {}; }
json Blockchain::eth_syncing(const json& request) { return {}; }
json Blockchain::eth_coinbase(const json& request) { return {}; }
json Blockchain::eth_blockNumber(const json& request) { return {}; }
json Blockchain::eth_call(const json& request) { return {}; }
json Blockchain::eth_estimateGas(const json& request) { return {}; }
json Blockchain::eth_gasPrice(const json& request) { return {}; }
json Blockchain::eth_feeHistory(const json& request) { return {}; }
json Blockchain::eth_getLogs(const json& request) { return {}; }
json Blockchain::eth_getBalance(const json& request) { return {}; }
json Blockchain::eth_getTransactionCount(const json& request) { return {}; }
json Blockchain::eth_getCode(const json& request) { return {}; }
json Blockchain::eth_sendRawTransaction(const json& request) { return {}; }
json Blockchain::eth_getTransactionByHash(const json& request) { return {}; }
json Blockchain::eth_getTransactionByBlockHashAndIndex(const json& request){ return {}; }
json Blockchain::eth_getTransactionByBlockNumberAndIndex(const json& request) { return {}; }
json Blockchain::eth_getTransactionReceipt(const json& request) { return {}; }
json Blockchain::eth_getUncleByBlockHashAndIndex() { return {}; }
