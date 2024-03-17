/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "broadcaster.h"
#include "blockchain.h"

void Broadcaster::validatorLoop() {
  Logger::logToDebug(LogType::INFO, Log::broadcaster, __func__, "Starting validator loop.");
  Validator me(Secp256k1::toAddress(Secp256k1::toUPub(this->blockchain_.getOptions().getValidatorPrivKey())));
  this->blockchain_.getState().rdposStartWorker();
  while (!this->blockchain_.getSyncer().isStopped()) {
    std::shared_ptr<const Block> latestBlock = this->blockchain_.getStorage().latest();

    // Check if validator is within the current validator list.
    const auto currentRandomList = this->blockchain_.getState().rdposGetRandomList();
    bool isBlockCreator = false;
    if (currentRandomList[0] == me) {
      isBlockCreator = true;
      this->doValidatorBlock();
    }

    if (this->blockchain_.getSyncer().isStopped()) return;
    if (!isBlockCreator) this->doValidatorTx();

    // Keep looping while we don't reach the latest block
    bool logged = false;
    while (latestBlock != this->blockchain_.getStorage().latest() && !this->blockchain_.getSyncer().isStopped()) {
      if (!logged) {
        Logger::logToDebug(LogType::INFO, Log::broadcaster, __func__, "Waiting for next block to be created.");
        logged = true;
      }
      // Wait for next block to be created.
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  }
}

void Broadcaster::nonValidatorLoop() const {
  // TODO: Improve tx broadcasting and syncing
  while (!this->blockchain_.getSyncer().isStopped()) {
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
}

void Broadcaster::doValidatorBlock() {
  // TODO: Improve this somehow.
  // Wait until we are ready to create the block
  bool logged = false;
  while (!this->blockchain_.getState().rdposCanCreateBlock()) {
    if (!logged) {
      logged = true;
      Logger::logToDebug(LogType::INFO, Log::broadcaster, __func__, "Waiting for rdPoS to be ready to create a block.");
    }
    if (this->blockchain_.getSyncer().isStopped()) return;
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  // Wait until we have at least one transaction in the state mempool.
  logged = false;
  while (this->blockchain_.getState().getMempoolSize() < 1) {
    if (!logged) {
      logged = true;
      Logger::logToDebug(LogType::INFO, Log::broadcaster, __func__, "Waiting for at least one transaction in the mempool.");
    }
    if (this->blockchain_.getSyncer().isStopped()) return;

    // Try to get transactions from the network.
    auto connectedNodesList = this->blockchain_.getP2P().getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      if (this->blockchain_.getSyncer().isStopped()) break;
      auto txList = this->blockchain_.getP2P().requestTxs(nodeId);
      if (this->blockchain_.getSyncer().isStopped()) break;
      for (auto const& tx : txList) {
        TxBlock txBlock(tx);
        this->blockchain_.getState().addTx(std::move(txBlock));
      }
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  // Create the block.
  if (this->blockchain_.getSyncer().isStopped()) return;
  auto mempool = this->blockchain_.getState().rdposGetMempool();
  auto randomList = this->blockchain_.getState().rdposGetRandomList();

  // Order the transactions in the proper manner.
  std::vector<TxValidator> randomHashTxs;
  std::vector<TxValidator> randomnessTxs;
  uint64_t i = 1;
  while (randomHashTxs.size() != this->blockchain_.getState().rdposGetMinValidators()) {
    for (const auto& [txHash, tx] : mempool) {
      if (this->blockchain_.getSyncer().isStopped()) return;
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
  if (this->blockchain_.getSyncer().isStopped()) return;

  // Create the block and append to all chains, we can use any storage for latest block.
  const std::shared_ptr<const Block> latestBlock = this->blockchain_.getStorage().latest();
  Block block(latestBlock->hash(), latestBlock->getTimestamp(), latestBlock->getNHeight() + 1);

  // Append transactions towards block.
  for (const auto& tx: randomHashTxs) block.appendTxValidator(tx);
  for (const auto& tx: randomnessTxs) block.appendTxValidator(tx);
  if (this->blockchain_.getSyncer().isStopped()) return;

  // Add transactions from state, sign, validate and process the block.
  this->blockchain_.getState().fillBlockWithTransactions(block);
  this->blockchain_.getState().rdposSignBlock(block);
  if (!this->blockchain_.getState().validateNextBlock(block)) {
    Logger::logToDebug(LogType::ERROR, Log::broadcaster, __func__, "Block is not valid!");
    throw DynamicException("Block is not valid!");
  }
  if (this->blockchain_.getSyncer().isStopped()) return;
  Hash latestBlockHash = block.hash();
  this->blockchain_.getState().processNextBlock(std::move(block));
  if (this->blockchain_.getStorage().latest()->hash() != latestBlockHash) {
    Logger::logToDebug(LogType::ERROR, Log::broadcaster, __func__, "Block is not valid!");
    throw DynamicException("Block is not valid!");
  }

  // Broadcast the block through P2P
  if (this->blockchain_.getSyncer().isStopped()) return;
  this->blockchain_.getP2P().broadcastBlock(this->blockchain_.getStorage().latest());
}

void Broadcaster::doValidatorTx() const {
  // There is nothing to do, validatorLoop will wait for the next block.
}

