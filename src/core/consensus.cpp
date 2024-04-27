/*
Copyright (c) [2023-2024] [Sparq Network]

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
    // Try to get more transactions from other nodes within the network
    auto connectedNodesList = this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      auto txList = this->p2p_.requestValidatorTxs(nodeId);
      if (this->stop_) return;
      for (auto const& tx : txList) this->state_.addValidatorTx(tx);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Validator ready to create a block");

  // Wait until we have all required transactions to create the block.
  auto waitForTxs = std::chrono::high_resolution_clock::now();
  bool logged = false;
  while (this->state_.getMempoolSize() < 1) {
    if (!logged) {
      logged = true;
      Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Waiting for at least one transaction in the mempool.");
    }
    if (this->stop_) return;

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
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  auto creatingBlock = std::chrono::high_resolution_clock::now();

  // Create the block.
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Ordering transactions and creating block");
  if (this->stop_) return;
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
  MutableBlock mutableBlock(latestBlock->getHash(), latestBlock->getTimestamp(), latestBlock->getNHeight() + 1);

  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Filling block with transactions.");
  // Append transactions towards block.
  for (const auto& tx: randomHashTxs) mutableBlock.appendTxValidator(tx);
  for (const auto& tx: randomnessTxs) mutableBlock.appendTxValidator(tx);
  if (this->stop_) return;

  // Add transactions from state, sign, validate and process the block.
  this->state_.fillBlockWithTransactions(mutableBlock);
  auto block = this->signBlock(mutableBlock);
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
  // TODO: this should go to its own class (Broadcaster)
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Broadcasting block.");
  if (this->stop_) return;
  this->p2p_.broadcastBlock(this->storage_.latest());
  auto end = std::chrono::high_resolution_clock::now();
  double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  double timeToConsensus = std::chrono::duration_cast<std::chrono::milliseconds>(waitForTxs - start).count();
  double timeToTxs = std::chrono::duration_cast<std::chrono::milliseconds>(creatingBlock - waitForTxs).count();
  double timeToBlock = std::chrono::duration_cast<std::chrono::milliseconds>(end - creatingBlock).count();
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
  // TODO: this should be in Broadcaster?
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Broadcasting randomHash transaction");
  this->state_.rdposAddValidatorTx(randomHashTx);
  this->p2p_.broadcastTxValidator(randomHashTx);

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
    // Try to get more transactions from other nodes within the network
    auto connectedNodesList = this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      if (this->stop_) return;
      auto txList = this->p2p_.requestValidatorTxs(nodeId);
      for (auto const& tx : txList) this->state_.addValidatorTx(tx);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Broadcasting random transaction");
  // Append and broadcast the randomness transaction.
  // TODO: this should be in Broadcaster?
  this->state_.addValidatorTx(seedTx);
  this->p2p_.broadcastTxValidator(seedTx);
}

FinalizedBlock Consensus::signBlock(MutableBlock &block) {
  uint64_t newTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::high_resolution_clock::now().time_since_epoch()
  ).count();
  return block.finalize(this->options_.getValidatorPrivKey(), newTimestamp);
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

