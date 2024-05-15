/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "consensus.h"
#include "blockchain.h"

void Consensus::validatorLoop() {
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Starting validator loop.");
  Validator me(Secp256k1::toAddress(Secp256k1::toUPub(this->options_.getValidatorPrivKey())));
  while (!this->stop_) {
    std::shared_ptr<const FinalizedBlock> latestBlock = this->storage_.latest();

    // Check if validator is within the current validator list.
    const auto currentRandomList = this->state_.rdposGetRandomList();
    bool isBlockCreator = false;
    if (currentRandomList[0] == me) {
      isBlockCreator = true;
      this->doValidatorBlock();
    }
    if (this->stop_) return;
    if (!isBlockCreator) this->doValidatorTx(latestBlock->getNHeight() + 1, me);

    // Keep looping while we don't reach the latest block
    bool logged = false;
    while (latestBlock == this->storage_.latest() && !this->stop_) {
      if (!logged) {
        Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Waiting for next block to be created.");
        logged = true;
      }
      // Wait for next block to be created.
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  }
}

void Consensus::doValidatorBlock() {
  // TODO: Improve this somehow.
  // Wait until we are ready to create the block
  auto start = std::chrono::high_resolution_clock::now();
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Block creator: waiting for txs");
  uint64_t validatorMempoolSize = 0;
  std::unique_ptr<uint64_t> lastLog = nullptr;
  while (validatorMempoolSize != this->state_.rdposGetMinValidators() * 2 && !this->stop_) {
    if (lastLog == nullptr || *lastLog != validatorMempoolSize) {
      lastLog = std::make_unique<uint64_t>(validatorMempoolSize);
      Logger::logToDebug(LogType::INFO, Log::consensus, __func__,
        "Block creator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool"
      );
    }
    validatorMempoolSize = this->state_.rdposGetMempoolSize();
    /*
      // TODO/REMOVE: There is no need for validator[0] to reach out for validatorTxs
      //   as those should be proactively broadcasted by the nodes that must produce them.
      //   As long as we don't discard the validatorTxs because they are out of order,
      //   trying to download them here should be unnecessary.

    // Try to get more transactions from other nodes within the network
    auto connectedNodesList = this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      auto txList = this->p2p_.requestValidatorTxs(nodeId);
      if (this->stop_) return;
      for (auto const& tx : txList) this->state_.addValidatorTx(tx);
    }
    */
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Validator ready to create a block");

  // ----------------------------------------------------------------------------------------------------------
  // ----------------------------------------------------------------------------------------------------------
  // TODO/REVIEW: This entire block of code should probably be deleted.
  //
  // The block proposer for the round (validator[0]) will not risk getting timed out/punished, so, people
  //   who run validators will simply attach a client to propose a dummy transaction that transfers 0.0001 coin
  //   between two accounts, instead of risk being timed-out by the other validators during transaction set
  //   synchronization and voting (or, more likely, fork the node software to generate a dummy transaction here
  //   and stuff it in the block)
  // Empty blocks are as valid as any block. All this does is create a liveness obstacle, which can ripple
  //   into network security and stability problems. A block should be produced when it is (real, wall-clock,
  //   network) time to produce a block.
  // This violates the Principle of Least Astonishment, where people are staring at a block explorer and
  //   wondering why the block height counter is stuck.
  // Also, a chain with empty blocks is economically dead; it has bigger problems than saving disk space
  //   by avoiding empty blocks.
  // ----------------------------------------------------------------------------------------------------------
  // ----------------------------------------------------------------------------------------------------------

  // Wait until we have all required transactions to create the block.
  auto waitForTxs = std::chrono::high_resolution_clock::now();
  bool logged = false;
  while (this->state_.getMempoolSize() < 1) {
    if (!logged) {
      logged = true;
      Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Waiting for at least one transaction in the mempool.");
    }
    if (this->stop_) return;
    /*
      // TODO/REVIEW: Removed code.
      // User transactions ("Block" Tx / normal TX / non-validator Tx) should be naturally flowing through the
      // network. If validator[0] does not have any, all it has to do is wait here for whatever transaction dissemination
      // mechanism is running in the protocol to get a transaction to appear in the mempool.
      // If a combination of node & test code is failing to provide user transactions for this loop to unblock itself, that
      // is a problem in the node and/or the test. Under no circumstance would we be pulling user transactions here. In that
      // case, looping forever here is the correct behavior as far as the Consensus component is concerned.

    // Try to get transactions from the network.
    auto connectedNodesList = this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Requesting txs...");
      if (this->stop_) break;
      auto txList = this->p2p_.requestTxs(nodeId);
      if (this->stop_) break;
      for (auto const& tx : txList) {
        TxBlock txBlock(tx);
        this->state_.addTx(std::move(txBlock));
      }
    }
    */
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
  // ----------------------------------------------------------------------------------------------------------
  // ----------------------------------------------------------------------------------------------------------

  if (this->stop_) return;

  // Create the block.
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Ordering transactions and creating block");
  auto creatingBlock = std::chrono::high_resolution_clock::now();
  auto mempool = this->state_.rdposGetMempool();
  auto randomList = this->state_.rdposGetRandomList();

  // Order the transactions in the proper manner.
  std::vector<TxValidator> randomHashTxs;
  std::vector<TxValidator> randomnessTxs;
  uint64_t i = 1;
  while (randomHashTxs.size() != this->state_.rdposGetMinValidators()) {
    for (const auto& [txHash, tx] : mempool) {
      if (this->stop_) return;
      // 0xcfffe746 == 3489654598
      if (tx.getFrom() == randomList[i] && tx.getFunctor().value == 3489654598) {
        randomHashTxs.emplace_back(tx);
        i++;
        break;
      }
    }
  }
  i = 1;
  while (randomnessTxs.size() != this->state_.rdposGetMinValidators()) {
    for (const auto& [txHash, tx] : mempool) {
      // 0x6fc5a2d6 == 1875223254
      if (tx.getFrom() == randomList[i] && tx.getFunctor().value == 1875223254) {
        randomnessTxs.emplace_back(tx);
        i++;
        break;
      }
    }
  }
  if (this->stop_) return;

  // Create the block and append to all chains, we can use any storage for latest block.
  const std::shared_ptr<const FinalizedBlock> latestBlock = this->storage_.latest();

  // Append all validator transactions to a single vector (Will be moved to the new block)
  std::vector<TxValidator> validatorTxs;
  for (const auto& tx: randomHashTxs) validatorTxs.emplace_back(tx);
  for (const auto& tx: randomnessTxs) validatorTxs.emplace_back(tx);
  if (this->stop_) return;

  // Get a copy of the mempool and current timestamp
  auto chainTxs = this->state_.getMempool();
  auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Create a new valid block.");
  auto block = FinalizedBlock::createNewValidBlock(std::move(chainTxs),
                                                   std::move(validatorTxs),
                                                   latestBlock->getHash(),
                                                   timestamp,
                                                   latestBlock->getNHeight() + 1,
                                                   this->options_.getValidatorPrivKey());
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Block created, validating.");
  if (!this->state_.validateNextBlock(block)) {
    Logger::logToDebug(LogType::ERROR, Log::consensus, __func__, "Block is not valid!");
    throw DynamicException("Block is not valid!");
  }
  if (this->stop_) return;
  Hash latestBlockHash = block.getHash();
  this->state_.processNextBlock(std::move(block));
  if (this->storage_.latest()->getHash() != latestBlockHash) {
    Logger::logToDebug(LogType::ERROR, Log::consensus, __func__, "Block is not valid!");
    throw DynamicException("Block is not valid!");
  }

  // Broadcast the block through P2P
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Broadcasting block.");
  if (this->stop_) return;
  this->p2p_.getBroadcaster().broadcastBlock(this->storage_.latest());
  auto end = std::chrono::high_resolution_clock::now();
  long double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  long double timeToConsensus = std::chrono::duration_cast<std::chrono::milliseconds>(waitForTxs - start).count();
  long double timeToTxs = std::chrono::duration_cast<std::chrono::milliseconds>(creatingBlock - waitForTxs).count();
  long double timeToBlock = std::chrono::duration_cast<std::chrono::milliseconds>(end - creatingBlock).count();
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__,
    "Block created in: " + std::to_string(duration) + "ms, " +
    "Time to consensus: " + std::to_string(timeToConsensus) + "ms, " +
    "Time to txs: " + std::to_string(timeToTxs) + "ms, " +
    "Time to block: " + std::to_string(timeToBlock) + "ms"
  );
}

void Consensus::doValidatorTx(const uint64_t& nHeight, const Validator& me) {
  Hash randomness = Hash::random();
  Hash randomHash = Utils::sha3(randomness.get());
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Creating random Hash transaction");
  Bytes randomHashBytes = Hex::toBytes("0xcfffe746");
  randomHashBytes.insert(randomHashBytes.end(), randomHash.get().begin(), randomHash.get().end());
  TxValidator randomHashTx(
    me.address(),
    randomHashBytes,
    this->options_.getChainID(),
    nHeight,
    this->options_.getValidatorPrivKey()
  );

  Bytes seedBytes = Hex::toBytes("0x6fc5a2d6");
  seedBytes.insert(seedBytes.end(), randomness.get().begin(), randomness.get().end());
  TxValidator seedTx(
    me.address(),
    seedBytes,
    this->options_.getChainID(),
    nHeight,
    this->options_.getValidatorPrivKey()
  );

  // Sanity check if tx is valid
  BytesArrView randomHashTxView(randomHashTx.getData());
  BytesArrView randomSeedTxView(seedTx.getData());
  if (Utils::sha3(randomSeedTxView.subspan(4)) != randomHashTxView.subspan(4)) {
    Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "RandomHash transaction is not valid!!!");
    return;
  }

  // Append to mempool and broadcast the transaction across all nodes.
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Broadcasting randomHash transaction");
  this->state_.rdposAddValidatorTx(randomHashTx);
  this->p2p_.getBroadcaster().broadcastTxValidator(randomHashTx);

  // Wait until we received all randomHash transactions to broadcast the randomness transaction
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Waiting for randomHash transactions to be broadcasted");
  uint64_t validatorMempoolSize = 0;
  std::unique_ptr<uint64_t> lastLog = nullptr;
  while (validatorMempoolSize < this->state_.rdposGetMinValidators() && !this->stop_) {
    if (lastLog == nullptr || *lastLog != validatorMempoolSize) {
      lastLog = std::make_unique<uint64_t>(validatorMempoolSize);
      Logger::logToDebug(LogType::INFO, Log::consensus, __func__,
        "Validator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool"
      );
    }
    validatorMempoolSize = this->state_.rdposGetMempoolSize();
    /*
      // TODO/REMOVE: There is no need for validator[0] to reach out for validatorTxs
      //   as those should be proactively broadcasted by the nodes that must produce them.
      //   As long as we don't discard the validatorTxs because they are out of order,
      //   trying to download them here should be unnecessary.

    // Try to get more transactions from other nodes within the network
    auto connectedNodesList = this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      if (this->stop_) return;
      auto txList = this->p2p_.requestValidatorTxs(nodeId);
      for (auto const& tx : txList) this->state_.addValidatorTx(tx);
    }
    */
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Broadcasting random transaction");
  // Append and broadcast the randomness transaction.
  this->state_.addValidatorTx(seedTx);
  this->p2p_.getBroadcaster().broadcastTxValidator(seedTx);
}

void Consensus::start() {
  if (this->state_.rdposGetIsValidator() && !this->loopFuture_.valid()) {
    this->loopFuture_ = std::async(std::launch::async, &Consensus::validatorLoop, this);
  }
}

void Consensus::stop() {
  if (this->loopFuture_.valid()) {
    this->stop_ = true;
    this->loopFuture_.wait();
    this->loopFuture_.get();
  }
}

