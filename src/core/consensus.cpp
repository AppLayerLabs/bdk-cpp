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
    std::shared_ptr<const Block> latestBlock = this->storage_.latest();

    // Check if validator is within the current validator list.
    const auto currentRandomList = this->state_.rdposGetRandomList();
    bool isBlockCreator = false;
    if (currentRandomList[0] == me) {
      isBlockCreator = true;
      this->doValidatorBlock();
    }
    if (this->stop_) return;
    if (!isBlockCreator) this->doValidatorTx();

    // Keep looping while we don't reach the latest block
    bool logged = false;
    while (latestBlock != this->storage_.latest() && !this->stop_) {
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
  bool logged = false;
  while (!this->canCreateBlock_) {
    if (!logged) {
      logged = true;
      Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Waiting for rdPoS to be ready to create a block.");
    }
    if (this->stop_) return;
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  // Wait until we have at least one transaction in the state mempool.
  logged = false;
  while (this->state_.getMempoolSize() < 1) {
    if (!logged) {
      logged = true;
      Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Waiting for at least one transaction in the mempool.");
    }
    if (this->stop_) return;

    // Try to get transactions from the network.
    auto connectedNodesList = this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
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

  // Create the block.
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
      if (tx.getFrom() == randomList[i] && tx.getFunctor() == Hex::toBytes("0xcfffe746")) {
        randomHashTxs.emplace_back(tx);
        i++;
        break;
      }
    }
  }
  i = 1;
  while (randomnessTxs.size() != this->state_.rdposGetMinValidators()) {
    for (const auto& [txHash, tx] : mempool) {
      if (tx.getFrom() == randomList[i] && tx.getFunctor() == Hex::toBytes("0x6fc5a2d6")) {
        randomnessTxs.emplace_back(tx);
        i++;
        break;
      }
    }
  }
  if (this->stop_) return;

  // Create the block and append to all chains, we can use any storage for latest block.
  const std::shared_ptr<const Block> latestBlock = this->storage_.latest();
  Block block(latestBlock->hash(), latestBlock->getTimestamp(), latestBlock->getNHeight() + 1);

  // Append transactions towards block.
  for (const auto& tx: randomHashTxs) block.appendTxValidator(tx);
  for (const auto& tx: randomnessTxs) block.appendTxValidator(tx);
  if (this->stop_) return;

  // Add transactions from state, sign, validate and process the block.
  this->state_.fillBlockWithTransactions(block);
  this->signBlock(block);
  if (!this->state_.validateNextBlock(block)) {
    Logger::logToDebug(LogType::ERROR, Log::consensus, __func__, "Block is not valid!");
    throw DynamicException("Block is not valid!");
  }
  if (this->stop_) return;
  Hash latestBlockHash = block.hash();
  this->state_.processNextBlock(std::move(block));
  if (this->storage_.latest()->hash() != latestBlockHash) {
    Logger::logToDebug(LogType::ERROR, Log::consensus, __func__, "Block is not valid!");
    throw DynamicException("Block is not valid!");
  }

  // Broadcast the block through P2P
  // TODO: this should go to its own class (Broadcaster)
  if (this->stop_) return;
  this->p2p_.broadcastBlock(this->storage_.latest());
}

void Consensus::doValidatorTx() const {
  // There is nothing to do, validatorLoop will wait for the next block.
}

void Consensus::doBlockCreation() {
  // TODO: add requesting transactions to other nodes when mempool is not filled up
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
    validatorMempoolSize = this->state_.rdposGetMempool().size();
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
  this->canCreateBlock_ = true; // Let everybody know that we are ready to create another block
}

void Consensus::doTxCreation(const uint64_t& nHeight, const Validator& me) {
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
    validatorMempoolSize = this->state_.rdposGetMempool().size();
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

bool Consensus::workerLoop() {
  Validator me(Secp256k1::toAddress(Secp256k1::toUPub(this->options_.getValidatorPrivKey())));
  const std::shared_ptr<const Block> latestBlock = this->storage_.latest();
  while (!this->stop_) {
    // Check if we are the validator required for signing the block.
    bool isBlockCreator = false;
    if (me == this->state_.rdposGetRandomList()[0]) {
      isBlockCreator = true;
      this->doBlockCreation();
    }

    // Check if we are one of the rdPoS that need to create random transactions.
    if (!isBlockCreator) {
      for (uint64_t i = 1; i <= this->state_.rdposGetMinValidators(); i++) {
        if (me == this->state_.rdposGetRandomList()[i]) {
          this->doTxCreation(latestBlock->getNHeight() + 1, me);
        }
      }
    }

    // After processing everything. wait until the new block is appended to the chain.
    std::unique_ptr<std::tuple<uint64_t, uint64_t, uint64_t>> lastLog = nullptr;
    while (latestBlock != this->storage_.latest() && !this->stop_) {
      if (lastLog == nullptr || std::get<0>(*lastLog) != latestBlock->getNHeight() ||
        std::get<1>(*lastLog) != this->storage_.latest()->getNHeight() ||
        std::get<2>(*lastLog) != this->state_.rdposGetMempool().size()
      ) {
        lastLog = std::make_unique<std::tuple<uint64_t, uint64_t, uint64_t>>(
          latestBlock->getNHeight(),
          this->storage_.latest()->getNHeight(),
          this->state_.rdposGetMempool().size()
        );
        Logger::logToDebug(LogType::INFO, Log::consensus, __func__,
          "Waiting for new block to be appended to the chain. (Height: "
          + std::to_string(latestBlock->getNHeight()) + ")" + " latest height: "
          + std::to_string(this->storage_.latest()->getNHeight())
        );
        Logger::logToDebug(LogType::INFO, Log::consensus, __func__,
          "Currently has " + std::to_string(this->state_.rdposGetMempool().size())
          + " transactions in mempool."
        );
      }
      uint64_t mempoolSize = this->state_.rdposGetMempool().size();
      // Always try to fill the mempool to 8 transactions
      if (mempoolSize < this->state_.rdposGetMinValidators()) {
        // Try to get more transactions from other nodes within the network
        auto connectedNodesList = this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
        for (auto const& nodeId : connectedNodesList) {
          if (latestBlock == this->storage_.latest() || this->stop_) break;
          auto txList = this->p2p_.requestValidatorTxs(nodeId);
          if (latestBlock == this->storage_.latest() || this->stop_) break;
          for (auto const& tx : txList) this->state_.addValidatorTx(tx);
        }
      }
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    if (isBlockCreator) this->canCreateBlock_ = false;
  }
  return true;
}

void Consensus::signBlock(Block &block) {
  uint64_t newTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::high_resolution_clock::now().time_since_epoch()
  ).count();
  block.finalize(this->options_.getValidatorPrivKey(), newTimestamp);
  this->canCreateBlock_ = false;
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

