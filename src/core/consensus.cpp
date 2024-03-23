/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "consensus.h"
#include "blockchain.h"

void Consensus::validatorLoop() {
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Starting validator loop.");
  Validator me(Secp256k1::toAddress(Secp256k1::toUPub(this->blockchain_.getOptions().getValidatorPrivKey())));
  while (!this->stop_) {
    std::shared_ptr<const Block> latestBlock = this->blockchain_.getStorage().latest();

    // Check if validator is within the current validator list.
    const auto currentRandomList = this->blockchain_.getState().rdposGetRandomList();
    bool isBlockCreator = false;
    if (currentRandomList[0] == me) {
      isBlockCreator = true;
      this->doValidatorBlock();
    }
    if (this->stop_) return;
    if (!isBlockCreator) this->doValidatorTx();

    // Keep looping while we don't reach the latest block
    bool logged = false;
    while (latestBlock != this->blockchain_.getStorage().latest() && !this->stop_) {
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
  while (this->blockchain_.getState().getMempoolSize() < 1) {
    if (!logged) {
      logged = true;
      Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Waiting for at least one transaction in the mempool.");
    }
    if (this->stop_) return;

    // Try to get transactions from the network.
    auto connectedNodesList = this->blockchain_.getP2P().getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      if (this->stop_) break;
      auto txList = this->blockchain_.getP2P().requestTxs(nodeId);
      if (this->stop_) break;
      for (auto const& tx : txList) {
        TxBlock txBlock(tx);
        this->blockchain_.getState().addTx(std::move(txBlock));
      }
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  // Create the block.
  if (this->stop_) return;
  auto mempool = this->blockchain_.getState().rdposGetMempool();
  auto randomList = this->blockchain_.getState().rdposGetRandomList();

  // Order the transactions in the proper manner.
  std::vector<TxValidator> randomHashTxs;
  std::vector<TxValidator> randomnessTxs;
  uint64_t i = 1;
  while (randomHashTxs.size() != this->blockchain_.getState().rdposGetMinValidators()) {
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
  while (randomnessTxs.size() != this->blockchain_.getState().rdposGetMinValidators()) {
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
  const std::shared_ptr<const Block> latestBlock = this->blockchain_.getStorage().latest();
  Block block(latestBlock->hash(), latestBlock->getTimestamp(), latestBlock->getNHeight() + 1);

  // Append transactions towards block.
  for (const auto& tx: randomHashTxs) block.appendTxValidator(tx);
  for (const auto& tx: randomnessTxs) block.appendTxValidator(tx);
  if (this->stop_) return;

  // Add transactions from state, sign, validate and process the block.
  this->blockchain_.getState().fillBlockWithTransactions(block);
  this->signBlock(block);
  if (!this->blockchain_.getState().validateNextBlock(block)) {
    Logger::logToDebug(LogType::ERROR, Log::consensus, __func__, "Block is not valid!");
    throw DynamicException("Block is not valid!");
  }
  if (this->stop_) return;
  Hash latestBlockHash = block.hash();
  this->blockchain_.getState().processNextBlock(std::move(block));
  if (this->blockchain_.getStorage().latest()->hash() != latestBlockHash) {
    Logger::logToDebug(LogType::ERROR, Log::consensus, __func__, "Block is not valid!");
    throw DynamicException("Block is not valid!");
  }

  // Broadcast the block through P2P
  // TODO: this should go to its own class (Broadcaster)
  if (this->stop_) return;
  this->blockchain_.getP2P().broadcastBlock(this->blockchain_.getStorage().latest());
}

void Consensus::doValidatorTx() const {
  // There is nothing to do, validatorLoop will wait for the next block.
}

void Consensus::doBlockCreation() {
  // TODO: add requesting transactions to other nodes when mempool is not filled up
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Block creator: waiting for txs");
  uint64_t validatorMempoolSize = 0;
  std::unique_ptr<uint64_t> lastLog = nullptr;
  while (validatorMempoolSize != this->blockchain_.getState().rdposGetMinValidators() * 2 && !this->stop_) {
    if (lastLog == nullptr || *lastLog != validatorMempoolSize) {
      lastLog = std::make_unique<uint64_t>(validatorMempoolSize);
      Logger::logToDebug(LogType::INFO, Log::consensus, __func__,
        "Block creator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool"
      );
    }
    validatorMempoolSize = this->blockchain_.getState().rdposGetMempool().size();
    // Try to get more transactions from other nodes within the network
    auto connectedNodesList = this->blockchain_.getP2P().getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      auto txList = this->blockchain_.getP2P().requestValidatorTxs(nodeId);
      if (this->stop_) return;
      for (auto const& tx : txList) this->blockchain_.getState().addValidatorTx(tx);
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
    this->blockchain_.getOptions().getChainID(),
    nHeight,
    this->blockchain_.getOptions().getValidatorPrivKey()
  );

  Bytes seedBytes = Hex::toBytes("0x6fc5a2d6");
  seedBytes.insert(seedBytes.end(), randomness.get().begin(), randomness.get().end());
  TxValidator seedTx(
    me.address(),
    seedBytes,
    this->blockchain_.getOptions().getChainID(),
    nHeight,
    this->blockchain_.getOptions().getValidatorPrivKey()
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
  this->blockchain_.getState().rdposAddValidatorTx(randomHashTx);
  this->blockchain_.getP2P().broadcastTxValidator(randomHashTx);

  // Wait until we received all randomHash transactions to broadcast the randomness transaction
  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Waiting for randomHash transactions to be broadcasted");
  uint64_t validatorMempoolSize = 0;
  std::unique_ptr<uint64_t> lastLog = nullptr;
  while (validatorMempoolSize < this->blockchain_.getState().rdposGetMinValidators() && !this->stop_) {
    if (lastLog == nullptr || *lastLog != validatorMempoolSize) {
      lastLog = std::make_unique<uint64_t>(validatorMempoolSize);
      Logger::logToDebug(LogType::INFO, Log::consensus, __func__,
        "Validator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool"
      );
    }
    validatorMempoolSize = this->blockchain_.getState().rdposGetMempool().size();
    // Try to get more transactions from other nodes within the network
    auto connectedNodesList = this->blockchain_.getP2P().getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      if (this->stop_) return;
      auto txList = this->blockchain_.getP2P().requestValidatorTxs(nodeId);
      for (auto const& tx : txList) this->blockchain_.getState().addValidatorTx(tx);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  Logger::logToDebug(LogType::INFO, Log::consensus, __func__, "Broadcasting random transaction");
  // Append and broadcast the randomness transaction.
  // TODO: this should be in Broadcaster?
  this->blockchain_.getState().addValidatorTx(seedTx);
  this->blockchain_.getP2P().broadcastTxValidator(seedTx);
}

bool Consensus::workerLoop() {
  Validator me(Secp256k1::toAddress(Secp256k1::toUPub(this->blockchain_.getOptions().getValidatorPrivKey())));
  const std::shared_ptr<const Block> latestBlock = this->blockchain_.getStorage().latest();
  while (!this->stop_) {
    // Check if we are the validator required for signing the block.
    bool isBlockCreator = false;
    if (me == this->blockchain_.getState().rdposGetRandomList()[0]) {
      isBlockCreator = true;
      this->doBlockCreation();
    }

    // Check if we are one of the rdPoS that need to create random transactions.
    if (!isBlockCreator) {
      for (uint64_t i = 1; i <= this->blockchain_.getState().rdposGetMinValidators(); i++) {
        if (me == this->blockchain_.getState().rdposGetRandomList()[i]) {
          this->doTxCreation(latestBlock->getNHeight() + 1, me);
        }
      }
    }

    // After processing everything. wait until the new block is appended to the chain.
    std::unique_ptr<std::tuple<uint64_t, uint64_t, uint64_t>> lastLog = nullptr;
    while (latestBlock != this->blockchain_.getStorage().latest() && !this->stop_) {
      if (lastLog == nullptr || std::get<0>(*lastLog) != latestBlock->getNHeight() ||
        std::get<1>(*lastLog) != this->blockchain_.getStorage().latest()->getNHeight() ||
        std::get<2>(*lastLog) != this->blockchain_.getState().rdposGetMempool().size()
      ) {
        lastLog = std::make_unique<std::tuple<uint64_t, uint64_t, uint64_t>>(
          latestBlock->getNHeight(),
          this->blockchain_.getStorage().latest()->getNHeight(),
          this->blockchain_.getState().rdposGetMempool().size()
        );
        Logger::logToDebug(LogType::INFO, Log::consensus, __func__,
          "Waiting for new block to be appended to the chain. (Height: "
          + std::to_string(latestBlock->getNHeight()) + ")" + " latest height: "
          + std::to_string(this->blockchain_.getStorage().latest()->getNHeight())
        );
        Logger::logToDebug(LogType::INFO, Log::consensus, __func__,
          "Currently has " + std::to_string(this->blockchain_.getState().rdposGetMempool().size())
          + " transactions in mempool."
        );
      }
      uint64_t mempoolSize = this->blockchain_.getState().rdposGetMempool().size();
      // Always try to fill the mempool to 8 transactions
      if (mempoolSize < this->blockchain_.getState().rdposGetMinValidators()) {
        // Try to get more transactions from other nodes within the network
        auto connectedNodesList = this->blockchain_.getP2P().getSessionsIDs(P2P::NodeType::NORMAL_NODE);
        for (auto const& nodeId : connectedNodesList) {
          if (latestBlock == this->blockchain_.getStorage().latest() || this->stop_) break;
          auto txList = this->blockchain_.getP2P().requestValidatorTxs(nodeId);
          if (latestBlock == this->blockchain_.getStorage().latest() || this->stop_) break;
          for (auto const& tx : txList) this->blockchain_.getState().addValidatorTx(tx);
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
  block.finalize(this->blockchain_.getOptions().getValidatorPrivKey(), newTimestamp);
  this->canCreateBlock_ = false;
}

void Consensus::start() {
  if (this->blockchain_.getState().rdposGetIsValidator() && !this->loopFuture_.valid()) {
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

