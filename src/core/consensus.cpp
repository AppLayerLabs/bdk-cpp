/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "consensus.h"

void Consensus::validatorLoop() {
  LOGINFO("Starting validator loop.");
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
        LOGDEBUG("Waiting for next block to be created.");
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
  LOGDEBUG("Block creator: waiting for txs");
  uint64_t validatorMempoolSize = 0;
  std::unique_ptr<uint64_t> lastLog = nullptr;
  while (validatorMempoolSize != this->state_.rdposGetMinValidators() * 2 && !this->stop_) {
    if (lastLog == nullptr || *lastLog != validatorMempoolSize) {
      lastLog = std::make_unique<uint64_t>(validatorMempoolSize);
      LOGDEBUG("Block creator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool");
    }
    validatorMempoolSize = this->state_.rdposGetMempoolSize();
    // Try to get more transactions from other nodes within the network
    for (const auto& nodeId : this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE)) {
      auto txList = this->p2p_.requestValidatorTxs(nodeId);
      if (this->stop_) return;
      for (const auto& tx : txList) this->state_.addValidatorTx(tx);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
  LOGDEBUG("Validator ready to create a block");

  // Wait until we have all required transactions to create the block.
  auto waitForTxs = std::chrono::high_resolution_clock::now();
  bool logged = false;
  while (this->state_.getMempoolSize() < 1) {
    if (!logged) {
      logged = true;
      LOGDEBUG("Waiting for at least one transaction in the mempool.");
    }
    if (this->stop_) return;

    // Try to get transactions from the network.
    for (const auto& nodeId : this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE)) {
      LOGDEBUG("Requesting txs...");
      if (this->stop_) break;
      auto txList = this->p2p_.requestTxs(nodeId);
      if (this->stop_) break;
      for (const auto& tx : txList) {
        TxBlock txBlock(tx);
        this->state_.addTx(std::move(txBlock));
      }
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  auto creatingBlock = std::chrono::high_resolution_clock::now();

  // Create the block.
  LOGDEBUG("Ordering transactions and creating block");
  if (this->stop_) return;
  auto mempool = this->state_.rdposGetMempool();
  const auto randomList = this->state_.rdposGetRandomList();

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
  // We need to filter chainTxs to only allow one transaction per address
  // Otherwise we will have a nonce mismatch
  std::unordered_map<Address, TxBlock, SafeHash> chainTxsMap;
  for (const auto& tx : chainTxs) {
    chainTxsMap.insert({tx.getFrom(), tx});
  }
  chainTxs.clear();
  // Now copy the map back to the vector
  for (const auto& [addr, tx] : chainTxsMap) {
    chainTxs.emplace_back(tx);
  }

  uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();

  // To create a valid block according to block validation rules, the
  //  timestamp provided to the new block must be equal or greater (>=)
  //  than the timestamp of the previous block.
  timestamp = std::max(timestamp, latestBlock->getTimestamp());

  LOGDEBUG("Create a new valid block.");
  auto block = FinalizedBlock::createNewValidBlock(
    std::move(chainTxs), std::move(validatorTxs), latestBlock->getHash(),
    timestamp, latestBlock->getNHeight() + 1, this->options_.getValidatorPrivKey()
  );
  LOGDEBUG("Block created, validating.");
  Hash latestBlockHash = block.getHash();
  if (
    BlockValidationStatus bvs = state_.tryProcessNextBlock(std::move(block));
    bvs != BlockValidationStatus::valid
  ) {
    LOGERROR("Block is not valid!");
    throw DynamicException("Block is not valid!");
  }
  if (this->stop_) return;
  if (this->storage_.latest()->getHash() != latestBlockHash) {
    LOGERROR("Block is not valid!");
    throw DynamicException("Block is not valid!");
  }

  // Broadcast the block through P2P
  LOGDEBUG("Broadcasting block.");
  if (this->stop_) return;
  this->p2p_.getBroadcaster().broadcastBlock(this->storage_.latest());
  auto end = std::chrono::high_resolution_clock::now();
  long double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  long double timeToConsensus = std::chrono::duration_cast<std::chrono::milliseconds>(waitForTxs - start).count();
  long double timeToTxs = std::chrono::duration_cast<std::chrono::milliseconds>(creatingBlock - waitForTxs).count();
  long double timeToBlock = std::chrono::duration_cast<std::chrono::milliseconds>(end - creatingBlock).count();
  LOGDEBUG("Block created in: " + std::to_string(duration) + "ms, " +
    "Time to consensus: " + std::to_string(timeToConsensus) + "ms, " +
    "Time to txs: " + std::to_string(timeToTxs) + "ms, " +
    "Time to block: " + std::to_string(timeToBlock) + "ms"
  );
}

void Consensus::doValidatorTx(const uint64_t& nHeight, const Validator& me) {
  Hash randomness = Hash::random();
  Hash randomHash = Utils::sha3(randomness);
  LOGDEBUG("Creating random Hash transaction");
  Bytes randomHashBytes = Hex::toBytes("0xcfffe746");
  randomHashBytes.insert(randomHashBytes.end(), randomHash.begin(), randomHash.end());
  TxValidator randomHashTx(
    me.address(),
    randomHashBytes,
    this->options_.getChainID(),
    nHeight,
    this->options_.getValidatorPrivKey()
  );

  Bytes seedBytes = Hex::toBytes("0x6fc5a2d6");
  seedBytes.insert(seedBytes.end(), randomness.begin(), randomness.end());
  TxValidator seedTx(
    me.address(),
    seedBytes,
    this->options_.getChainID(),
    nHeight,
    this->options_.getValidatorPrivKey()
  );

  // Sanity check if tx is valid
  bytes::View randomHashTxView(randomHashTx.getData());
  bytes::View randomSeedTxView(seedTx.getData());
  if (Utils::sha3(randomSeedTxView.subspan(4)) != Hash(randomHashTxView.subspan(4))) {
    LOGDEBUG("RandomHash transaction is not valid!!!");
    return;
  }

  // Append to mempool and broadcast the transaction across all nodes.
  LOGDEBUG("Broadcasting randomHash transaction");
  this->state_.rdposAddValidatorTx(randomHashTx);
  this->p2p_.getBroadcaster().broadcastTxValidator(randomHashTx);

  // Wait until we received all randomHash transactions to broadcast the randomness transaction
  LOGDEBUG("Waiting for randomHash transactions to be broadcasted");
  uint64_t validatorMempoolSize = 0;
  std::unique_ptr<uint64_t> lastLog = nullptr;
  while (validatorMempoolSize < this->state_.rdposGetMinValidators() && !this->stop_) {
    if (lastLog == nullptr || *lastLog != validatorMempoolSize) {
      lastLog = std::make_unique<uint64_t>(validatorMempoolSize);
      LOGDEBUG("Validator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool");
    }
    validatorMempoolSize = this->state_.rdposGetMempoolSize();
    // Try to get more transactions from other nodes within the network
    for (const auto& nodeId : this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE)) {
      if (this->stop_) return;
      auto txList = this->p2p_.requestValidatorTxs(nodeId);
      for (const auto& tx : txList) this->state_.addValidatorTx(tx);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  LOGDEBUG("Broadcasting random transaction");
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

