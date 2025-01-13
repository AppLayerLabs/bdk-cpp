/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "blockchain.h"

#include "../utils/logger.h"

#define NODE_DATABASE_DIRECTORY_SUFFIX "/db/"

Blockchain::Blockchain(const std::string& blockchainPath, std::string instanceId)
  : instanceId_(instanceId),
    options_(Options::fromFile(blockchainPath)),
    comet_(this, instanceId, options_),
    state_(*this),
    storage_(*this),
    http_(options_.getHttpPort(), *this)//,
    //db_(options_.getRootPath() + NODE_DATABASE_DIRECTORY_SUFFIX)
{
}

Blockchain::Blockchain(const Options& options, const std::string& blockchainPath, std::string instanceId)
  : instanceId_(instanceId),
    options_(options),
    comet_(this, instanceId, options_),
    state_(*this),
    storage_(*this),
    http_(options_.getHttpPort(), *this)//,
    //db_(options_.getRootPath() + NODE_DATABASE_DIRECTORY_SUFFIX)
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

  // Wait for Comet to be in RUNNING state, since that is required
  // for e.g. Comet::sendTransaction() to succeed.
  this->comet_.setPauseState(CometState::RUNNING);
  this->comet_.start();
  std::string cometErr = this->comet_.waitPauseState(10000);
  if (cometErr != "") {
    throw DynamicException("Error while waiting for CometBFT: " + cometErr);
  }
  this->comet_.setPauseState();

  this->http_.start();
}

void Blockchain::stop() {
  this->http_.stop();
  this->comet_.stop();
}

std::shared_ptr<const FinalizedBlock> Blockchain::latest() const {
  return latest_.load();
}

// ------------------------------------------------------------------
// CometListener
// ------------------------------------------------------------------

void Blockchain::initChain(
  const uint64_t genesisTime, const std::string& chainId, const Bytes& initialAppState, const uint64_t initialHeight,
  const std::vector<CometValidatorUpdate>& initialValidators, Bytes& appHash
) {
  // TODO: Ensure atoi(chainId) == this->options_.getChainID() (should be the case)

  // For now, the validator set is fixed on genesis and never changes.
  // TODO: When we get to validator set changes via governance, validators_ will have to be
  //   updated via incomingBlock(validatorUpdates).
  validators_ = initialValidators;

  // Initialize the machine state on InitChain.
  // State is not RAII. We are not creating the State instance here.
  // State is created in a pre-comet-consensus, default state given by the BDK, and is initialized here to actual
  //   comet genesis state.
  // TODO: replace this with a call to a private initState() function (Blockchain is friend of State).
  std::unique_lock<std::shared_mutex> lock(state_.stateMutex_);

  // Unfortunately, CometBFT set the state height counter to the height for which you
  // are waiting a block for. The default initial height is 1, not 0 (0 is invalid in
  // cometBFT). However, we do use height 0 to mean the state is at genesis and waiting
  // for the first actual block (with height 1), so we need to fix this on our side.
  state_.height_ = initialHeight - 1;

  LOGDEBUG("Blockchain::initChain(): Height = " + std::to_string(initialHeight));
  state_.timeMicros_ = genesisTime * 1'000'000; // genesisTime is in seconds, so convert to microseconds
  // TODO: If we have support for initialAppState, apply it here, or load it from a BDK side channel
  //   like a genesis State dump/snapshot.
}

void Blockchain::checkTx(const Bytes& tx, int64_t& gasWanted, bool& accept)
{
  // TODO/REVIEW: It is possible that we will keep our own view of the mempool, or our idea
  // of what is in the cometbft mempool, in such a way that we'd know that transactions for
  // some account and nonce, nonce+1, nonce+2 are already there for example (because they have
  // been accepted on our side), so when we see a nonce+3 here for the same account, we know,
  // by looking up on that mempool cache, that this is valid, even though the account in State
  // is still at "nonce". We cannot, unfortunately, e.g. make RPC calls to cometbft from here
  // to poke at the cometbft mempool because all ABCI methods should return "immediately" (if
  // RPC queries are being made to explore the mempool, that's done by some internal worker
  // thread instead, and here we'd just be reading what it has gathered so far).

  // Simply parse and validate the transaction in isolation (this is just checking for
  // an exact nonce match with the tx sender account).
  try {
    TxBlock parsedTx(tx, options_.getChainID());
    accept = state_.validateTransaction(parsedTx);
  } catch (const std::exception& ex) {
    LOGDEBUG("ERROR: Blockchain::checkTx(): " + std::string(ex.what()));
    accept = false;
  }
}

void Blockchain::incomingBlock(
  const uint64_t syncingToHeight, std::unique_ptr<CometBlock> block, Bytes& appHash,
  std::vector<CometExecTxResult>& txResults, std::vector<CometValidatorUpdate>& validatorUpdates
) {
  try {
    FinalizedBlock finBlock = FinalizedBlock::fromCometBlock(*block);
    latest_.store(std::make_shared<const FinalizedBlock>(finBlock));
    if (!state_.validateNextBlock(finBlock)) {
      // Should never happen.
      // REVIEW: in fact, we shouldn't even need to verify the block at this point?
      LOGFATALP_THROW("Invalid block.");
    } else {
      // Update the sha3 --> sha256 txhash interop map
      // FIXME/TODO: we will need a strategy to be able to clean up tx hash mappings
      //  for old blocks, such as adding a bucket ID prefix to each key, then looking
      //  up a hash multiple times in all buckets.
      for (uint32_t i = 0; i < block->txs.size(); ++i) {
        Hash txHashSha3 = Utils::sha3(block->txs[i]);
        Hash txHashSha256 = Utils::sha256(block->txs[i]);
        storage_.putTxMap(txHashSha3, txHashSha256);
      }
      // Advance machine state
      std::vector<bool> succeeded;
      std::vector<uint64_t> gasUsed;
      state_.processBlock(finBlock, succeeded, gasUsed);

      // Fill in the txResults that get sent to cometbft for storage
      for (uint32_t i = 0; i < block->txs.size(); ++i) {
        CometExecTxResult txRes;
        txRes.code = succeeded[i] ? 0 : 1;
        txRes.gasUsed = gasUsed[i];
        txRes.gasWanted = static_cast<uint64_t>(finBlock.getTxs()[i].getGasLimit());
        //txRes.output = Bytes... FIXME: transaction execution result/return arbitrary bytes
        //in ContractHost::execute() there's an "output" var generated in the EVM code branch,
        //  but not in the CPP contract case branch
        txResults.emplace_back(txRes);
      }
    }
  } catch (const std::exception& ex) {
    // We need to fail the blockchain node (fatal)
    LOGFATALP_THROW("FATAL: Blockchain::incomingBlock(): " + std::string(ex.what()));
  }
}

void Blockchain::buildBlockProposal(
  const uint64_t maxTxBytes, const CometBlock& block, bool& noChange, std::vector<size_t>& txIds
) {
  // TODO: exclude invalid transactions (because invalid nonce or no ability to pay)
  // TODO: reorder transactions to make the nonce valid (say n, n+1, n+2 txs on same account but out of order)
  noChange = true;
}

void Blockchain::validateBlockProposal(const CometBlock& block, bool& accept) {
  // FIXME/TODO: Validate all transactions in sequence (context-aware)
  // For now, just validate all of the transactions in isolation (this is insufficient!)
  for (const auto& tx : block.txs) {
    try {
      TxBlock parsedTx(tx, options_.getChainID());
      if (!state_.validateTransaction(parsedTx)) {
        accept = false;
        return;
      }
    } catch (const std::exception& ex) {
      LOGDEBUG("ERROR: Blockchain::validateBlockProposal(): " + std::string(ex.what()));
      accept = false;
      return;
    }
  }
  accept = true;
}

void Blockchain::getCurrentState(uint64_t& height, Bytes& appHash, std::string& appSemVer, uint64_t& appVersion) {
  // TODO: return our machine state
  //       state_.height_, the state root hash, and version info
  height = state_.getHeight();

  // FIXME/TODO: fetch the state root hash (account state hash? not the same?)
  appHash = Hash().asBytes();

  // TODO/REVIEW: Not sure if we should set the BDK version here, since behavior might not change
  // If this is for display and doesn't trigger some cometbft behavior, then this can be the BDK version
  appSemVer = "1.0.0";

  // TODO: This for sure just changes (is incremented) when we change the behavior in a new BDK release
  appVersion = 0;
}

void Blockchain::getBlockRetainHeight(uint64_t& height) {
  // TODO: automatic block history pruning
  height = 0;
}

void Blockchain::currentCometBFTHeight(const uint64_t height) {
  // TODO: here, we must ensure that our state_.height_ CANNOT be greater than height
  //       it must either be exactly height, or if < height, we'll need to ask for blocks to be replayed
}

void Blockchain::sendTransactionResult(const uint64_t tId, const bool success, const json& response, const std::string& txHash, const Bytes& tx) {
}

void Blockchain::checkTransactionResult(const uint64_t tId, const bool success, const json& response, const std::string& txHash) {
}

void Blockchain::rpcAsyncCallResult(const uint64_t tId, const bool success, const json& response, const std::string& method, const json& params) {
}

void Blockchain::cometStateTransition(const CometState newState, const CometState oldState) {
  // TODO: trace log
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
