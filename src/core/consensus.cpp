/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "consensus.h"
#include "blockchain.h"
#include "bytes/random.h"

void Consensus::validatorLoop() {
  LOGINFO("Starting validator loop.");
  uint64_t loop = this->storage_.latest()->getNHeight() + 1;
  Validator me(Secp256k1::toAddress(Secp256k1::toUPub(this->options_.getValidatorPrivKey())));
  std::unordered_map<Validator, PrivKey, SafeHash> validators;
  validators.insert({me, this->options_.getValidatorPrivKey()});
  auto extraValidators = this->options_.getExtraValidators();
  if (extraValidators.empty()) {
    // If there are no extra validators, we are done.
    LOGINFO("No extra validators found.");
    return;
  }
  for (const auto& validator : this->options_.getExtraValidators()) {
    validators.insert({Secp256k1::toAddress(Secp256k1::toUPub(validator)), validator});
  }
  while (!this->stopConsensus_) {
    Utils::safePrint("Validator: " + me.hex(true).get() + " Loop: " + std::to_string(loop++));
    auto latestBlock = this->storage_.latest();
    auto latestBlockHeight = latestBlock->getNHeight();

    // Check if validator is within the current validator list.
    Utils::safePrint("Getting the current validators list.");
    const auto currentRandomList = this->state_.rdposGetRandomList();
    // Check which ones of the validators is the block creator (currentRandomList[0])
    PrivKey blockCreatorPrivKey;
    Utils::safePrint("Getting the block creator for block height: " + std::to_string(latestBlockHeight+1));
    for (const auto& [validator, privKey] : validators) {
      if (validator == currentRandomList[0]) {
        blockCreatorPrivKey = privKey;
        break;
      }
    }
    if (!blockCreatorPrivKey) {
      Utils::safePrint("Validator not in the current validator list.");
      throw DynamicException("Validator not in the current validator list.");
    }
    // Now, make the list of the validators that are going to be signing the block, based on the size of validators.
    auto validatorSetSize = this->options_.getMinValidators();
    // Make sure that the random list is at least +1 the validatorSetSize
    if (currentRandomList.size() <= validatorSetSize) {
      Utils::safePrint("Random list size is less than the minimum validators.");
      throw DynamicException("Random list size is less than the minimum validators.");
    }

    std::vector<PrivKey> validatorsToSign;
    for (uint64_t i = 0; i < validatorSetSize; i++) {
      for (const auto& [validator, privKey] : validators) {
        if (validator == currentRandomList[i+1]) {
          validatorsToSign.push_back(privKey);
          break;
        }
      }
    }
    if (validatorsToSign.size() != validatorSetSize) {
      Utils::safePrint("Validator set size mismatch.");
      throw DynamicException("Validator set size mismatch.");
    }

    // Now, create the validator transactions
    // Validator transactions must be within the validatorToSign order.
    Utils::safePrint("Creating validator transactions.");
    std::vector<TxValidator> hashTxs;
    std::vector<TxValidator> seedTxs;
    for (uint64_t i = 0; i < validatorsToSign.size(); ++i) {
      Hash randomness = bytes::random();
      Hash randomHash = Utils::sha3(randomness);
      Bytes randomHashBytes = Hex::toBytes("0xcfffe746");
      randomHashBytes.insert(randomHashBytes.end(), randomHash.begin(), randomHash.end());
      hashTxs.emplace_back(
        Secp256k1::toAddress(Secp256k1::toUPub(validatorsToSign[i])),
        randomHashBytes,
        this->options_.getChainID(),
        latestBlockHeight + 1,
        validatorsToSign[i]
      );
      Bytes seedBytes = Hex::toBytes("0x6fc5a2d6");
      seedBytes.insert(seedBytes.end(), randomness.begin(), randomness.end());
      seedTxs.emplace_back(
        Secp256k1::toAddress(Secp256k1::toUPub(validatorsToSign[i])),
        seedBytes,
        this->options_.getChainID(),
        latestBlockHeight + 1,
        validatorsToSign[i]
      );
    }
    Utils::safePrint("Validator transactions created.");
    // Now, move the trasactions to the final vector (firstly the hashTxs, then the seedTxs)
    std::vector<TxValidator> validatorTxs;
    for (const auto& tx: hashTxs) validatorTxs.emplace_back(tx);
    for (const auto& tx: seedTxs) validatorTxs.emplace_back(tx);
    if (validatorTxs.size() != validatorSetSize * 2) {
      Utils::safePrint("Validator transaction size mismatch, wanted: " + std::to_string(validatorSetSize * 2) + " got: " + std::to_string(validatorTxs.size()));
      throw DynamicException("Validator transaction size mismatch.");
    }
    // Check the ordering, validatorTxs[i] getFrom must be equal to currentRandomList[i]
    // Also, validatorTxs[i+validatorSetSize] getFrom must be equal to currentRandomList[i]
    for (uint64_t i = 0; i < validatorSetSize; i++) {
      if (validatorTxs[i].getFrom() != currentRandomList[i+1]) {
        Utils::safePrint("Validator transaction ordering mismatch.");
        throw DynamicException("Validator transaction ordering mismatch.");
      }
      if (validatorTxs[i + validatorSetSize].getFrom() != currentRandomList[i+1]) {
        Utils::safePrint("Validator transaction ordering mismatch.");
        throw DynamicException("Validator transaction ordering mismatch.");
      }
    }

    auto waitForTxs = std::chrono::high_resolution_clock::now();
    bool logged = false;
    while (this->state_.getMempoolSize() < 1) {
      if (!logged) {
        logged = true;
        Utils::safePrint("Waiting for at least one transaction in the mempool.");
      }
      if (this->stopConsensus_) return;
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    // Finally, create the block
    Utils::safePrint("Creating block.");

    // Get a copy of the mempool and current timestamp
    auto chainTxs = this->state_.getMempool();
    // We need to filter chainTxs to only allow one transaction per address
    // Otherwise we will have a nonce mismatch
    std  ::unordered_map<Address, TxBlock, SafeHash> chainTxsMap;
    for (const auto& tx : chainTxs) {
      // We need to actually check which tx has the greatest fee spent
      auto txIt = chainTxsMap.find(tx.getFrom());
      if (txIt == chainTxsMap.end()) {
        chainTxsMap.insert({tx.getFrom(), tx});
      } else {
        if (tx.getMaxFeePerGas() > txIt->second.getMaxFeePerGas()) {
          chainTxsMap.insert_or_assign(tx.getFrom(), tx);
        }
      }
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

    Utils::safePrint("Create a new valid block.");
    auto block = FinalizedBlock::createNewValidBlock(
      std::move(chainTxs), std::move(validatorTxs), latestBlock->getHash(),
      timestamp, latestBlock->getNHeight() + 1, blockCreatorPrivKey
    );
    Utils::safePrint("Block created, validating.");
    Hash latestBlockHash = block.getHash();
    if (
      BlockValidationStatus bvs = state_.tryProcessNextBlock(std::move(block));
      bvs != BlockValidationStatus::valid
    ) {
      Utils::safePrint("Block is not valid!");
      throw DynamicException("Block is not valid!");
    }
    if (this->stopConsensus_) return;
    if (this->storage_.latest()->getHash() != latestBlockHash) {
      Utils::safePrint("Block is not valid!");
      throw DynamicException("Block is not valid!");
    }

    // Broadcast the block through P2P
    Utils::safePrint("Broadcasting block.");
    if (this->stopConsensus_) return;
    this->p2p_.getBroadcaster().broadcastBlock(this->storage_.latest());


    // Keep looping while we don't reach the latest block
    logged = false;
    while (latestBlockHeight == this->storage_.latest()->getNHeight() && !this->stopConsensus_) {
      if (!logged) {
        Utils::safePrint("Waiting for next block to be created.");
        Utils::safePrint("Validator: " + me.hex(true).get() + " Waiting for next block to be created.");
        logged = true;
      }
      // Wait for next block to be created.
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  }
}

void Consensus::pullerLoop() {
  // List of all current existing requests towards other nodes. A list for TxBlock and a list for TxValidator.
  std::unordered_map<P2P::NodeID, std::future<std::vector<TxBlock>>, SafeHash> txBlockRequests;
  while (!this->stopPuller_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    // First, lets get a list of all nodes we are connected to.
    auto nodes = this->p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    // Now, we remove from the requests map all nodes that are not connected anymore.
    for (auto it = txBlockRequests.begin(); it != txBlockRequests.end();) {
      if (std::find(nodes.begin(), nodes.end(), it->first) == nodes.end()) {
        it = txBlockRequests.erase(it);
      } else {
        ++it;
      }
    }

    // Check if the latest block is older than 100ms. If yes, attempt to sync blocks.
    {
      // Get the timestamp of the latest known block
      auto latestBlock = this->storage_.latest();
      auto lastBlockTimestamp = latestBlock->getTimestamp();
      // Get current system time
      auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      // Compute the age of the block in milliseconds
      auto blockAge = now - lastBlockTimestamp;
      if (blockAge > 100000) {
        // We are considered "behind" and need to sync blocks.
        // Get connected nodes with their info
        auto connected = this->p2p_.getNodeConns().getConnected();
        if (!connected.empty()) {
          // Find the node with the highest block height
          std::pair<P2P::NodeID, uint64_t> highestNode = {P2P::NodeID{}, 0};
          for (auto & [nodeId, nodeInfo] : connected) {
            if (nodeInfo.latestBlockHeight() > highestNode.second) {
              highestNode = {nodeId, nodeInfo.latestBlockHeight()};
            }
          }
          auto currentNHeight = latestBlock->getNHeight();
          // If we are not synced
          if (highestNode.second > currentNHeight) {
            Utils::safePrint("Block is older than 100ms, attempting to sync blocks.");
            Utils::safePrint("Highest node with height: " + std::to_string(highestNode.second));
            // For example:
            uint64_t blocksPerRequest = 100;
            uint64_t bytesPerRequestLimit = 1'000'000;
            auto downloadNHeight = currentNHeight + 1;
            auto downloadNHeightEnd = downloadNHeight + blocksPerRequest - 1;

            // Request the next range of blocks from the best node
            Utils::safePrint("Requesting blocks [" + std::to_string(downloadNHeight) + "," + std::to_string(downloadNHeightEnd) + "]");
            auto result = this->p2p_.requestBlock(
              highestNode.first, downloadNHeight, downloadNHeightEnd, bytesPerRequestLimit
            );
            Utils::safePrint("Received " + std::to_string(result.size()));

            // If we got blocks, process them
            if (!result.empty()) {
              try {
                for (auto & block : result) {
                  this->state_.tryProcessNextBlock(std::move(block));
                  ++downloadNHeight;
                }
              } catch (std::exception &e) {
                // We actually don't do anything here, because broadcast might have received a block
              }
            }
          }
        } else {
          // Not connected to any node? just wait for the next loop...
          continue;
        }
      }
    }

    // Now, we request transactions from all nodes that we are connected to AND we don't have a request for.
    for (const auto& nodeId : nodes) {
      if (txBlockRequests.find(nodeId) == txBlockRequests.end()) {
        txBlockRequests[nodeId] = std::async(std::launch::async, &P2P::ManagerNormal::requestTxs, &this->p2p_, nodeId);
      }
    }
    // Loop through all the current requests **without blocking**.
    for (auto it = txBlockRequests.begin(); it != txBlockRequests.end();) {
      auto& future = it->second;
      if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        auto txList = future.get();
        for (const auto& tx : txList) {
          TxBlock txBlock(tx);
          this->state_.addTx(std::move(txBlock));
        }
        // Remove the request from the list.
        it = txBlockRequests.erase(it);
      } else {
        ++it;
      }
    }
  }
  // Make sure to exit all the futures otherwise we will have orphaned threads!
  for (auto& [nodeId, future] : txBlockRequests) {
    future.wait();
    future.get();
  }
}


void Consensus::doValidatorBlock() {
  // TODO: Improve this somehow.
  // Wait until we are ready to create the block
  auto start = std::chrono::high_resolution_clock::now();
  LOGDEBUG("Block creator: waiting for txs");
  uint64_t validatorMempoolSize = 0;
  std::unique_ptr<uint64_t> lastLog = nullptr;
  while (validatorMempoolSize != this->state_.rdposGetMinValidators() * 2 && !this->stopConsensus_) {
    if (lastLog == nullptr || *lastLog != validatorMempoolSize) {
      lastLog = std::make_unique<uint64_t>(validatorMempoolSize);
      LOGDEBUG("Block creator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool");
    }
    validatorMempoolSize = this->state_.rdposGetMempoolSize();
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
    if (this->stopConsensus_) return;
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  auto creatingBlock = std::chrono::high_resolution_clock::now();

  // Create the block.
  LOGDEBUG("Ordering transactions and creating block");
  if (this->stopConsensus_) return;
  auto mempool = this->state_.rdposGetMempool();
  const auto randomList = this->state_.rdposGetRandomList();

  // Order the transactions in the proper manner.
  std::vector<TxValidator> randomHashTxs;
  std::vector<TxValidator> randomnessTxs;
  uint64_t i = 1;
  while (randomHashTxs.size() != this->state_.rdposGetMinValidators()) {
    for (const auto& [txHash, tx] : mempool) {
      if (this->stopConsensus_) return;
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
  if (this->stopConsensus_) return;

  // Create the block and append to all chains, we can use any storage for latest block.
  const std::shared_ptr<const FinalizedBlock> latestBlock = this->storage_.latest();

  // Append all validator transactions to a single vector (Will be moved to the new block)
  std::vector<TxValidator> validatorTxs;
  for (const auto& tx: randomHashTxs) validatorTxs.emplace_back(tx);
  for (const auto& tx: randomnessTxs) validatorTxs.emplace_back(tx);
  if (this->stopConsensus_) return;

  // Get a copy of the mempool and current timestamp
  auto chainTxs = this->state_.getMempool();
  // We need to filter chainTxs to only allow one transaction per address
  // Otherwise we will have a nonce mismatch
  std::unordered_map<Address, TxBlock, SafeHash> chainTxsMap;
  for (const auto& tx : chainTxs) {
    // We need to actually check which tx has the greatest fee spent
    auto txIt = chainTxsMap.find(tx.getFrom());
    if (txIt == chainTxsMap.end()) {
      chainTxsMap.insert({tx.getFrom(), tx});
    } else {
      if (tx.getMaxFeePerGas() > txIt->second.getMaxFeePerGas()) {
        chainTxsMap.insert_or_assign(tx.getFrom(), tx);
      }
    }
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
  if (this->stopConsensus_) return;
  if (this->storage_.latest()->getHash() != latestBlockHash) {
    LOGERROR("Block is not valid!");
    throw DynamicException("Block is not valid!");
  }

  // Broadcast the block through P2P
  LOGDEBUG("Broadcasting block.");
  if (this->stopConsensus_) return;
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
  Utils::safePrint("Validator: " + me.hex(true).get() + " doValidatorTx nHeight: " + std::to_string(nHeight));
  Hash randomness = bytes::random();
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
  View<Bytes> randomHashTxView(randomHashTx.getData());
  View<Bytes> randomSeedTxView(seedTx.getData());
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
  while (validatorMempoolSize < this->state_.rdposGetMinValidators() && !this->stopConsensus_) {
    if (lastLog == nullptr || *lastLog != validatorMempoolSize) {
      lastLog = std::make_unique<uint64_t>(validatorMempoolSize);
      LOGDEBUG("Validator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool");
    }
    validatorMempoolSize = this->state_.rdposGetMempoolSize();
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
  if (!this->pullFuture_.valid()) {
    this->pullFuture_ = std::async(std::launch::async, &Consensus::pullerLoop, this);
  }
}

void Consensus::stop() {
  if (this->loopFuture_.valid()) {
    Utils::safePrint("Stopping this->loopFuture_");
    this->stopConsensus_ = true;
    this->loopFuture_.wait();
    this->loopFuture_.get();
  }
  if (this->pullFuture_.valid()) {
    Utils::safePrint("Stopping this->pullFuture_");
    this->stopPuller_ = true;
    this->pullFuture_.wait();
    this->pullFuture_.get();
  }
}

