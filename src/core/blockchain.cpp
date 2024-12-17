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

// ------------------------------------------------------------------
// CometListener
// ------------------------------------------------------------------

void Blockchain::initChain(
  const uint64_t genesisTime, const std::string& chainId, const Bytes& initialAppState, const uint64_t initialHeight,
  const std::vector<CometValidatorUpdate>& initialValidators, Bytes& appHash
) {
}

void Blockchain::checkTx(const Bytes& tx, int64_t& gasWanted, bool& accept)
{
}

void Blockchain::incomingBlock(
  const uint64_t height, const uint64_t syncingToHeight, const std::vector<Bytes>& txs, const Bytes& proposerAddr, const uint64_t timeNanos,
  Bytes& appHash, std::vector<CometExecTxResult>& txResults, std::vector<CometValidatorUpdate>& validatorUpdates
) {
}

void Blockchain::buildBlockProposal(
  const uint64_t height, const uint64_t maxTxBytes, const uint64_t timeNanos,
  const std::vector<Bytes>& txs, std::unordered_set<size_t>& delTxIds
) {
}

void Blockchain::validateBlockProposal(const uint64_t height, const std::vector<Bytes>& txs, bool& accept) {
}

void Blockchain::getCurrentState(uint64_t& height, Bytes& appHash, std::string& appSemVer, uint64_t& appVersion) {
}

void Blockchain::getBlockRetainHeight(uint64_t& height) {
}

void Blockchain::currentCometBFTHeight(const uint64_t height) {
}

void Blockchain::sendTransactionResult(const uint64_t tId, const Bytes& tx, const bool success, const std::string& txHash, const json& response) {
}

void Blockchain::checkTransactionResult(const uint64_t tId, const std::string& txHash, const bool success, const json& response) {
}

void Blockchain::rpcAsyncCallResult(const uint64_t tId, const std::string& method, const json& params, const bool success, const json& response) {
}

void Blockchain::cometStateTransition(const CometState newState, const CometState oldState) {
}

// ------------------------------------------------------------------
// NodeRPCInterface
// ------------------------------------------------------------------

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
