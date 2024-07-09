/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "storage.h"

#include "boost/asio/post.hpp"
#include "bytes/join.h"

static void storeBlock(DB& db, const FinalizedBlock& block) {
  DBBatch batch;

  batch.push_back(block.getHash(), block.serializeBlock(), DBPrefix::blocks);
  batch.push_back(Utils::uint64ToBytes(block.getNHeight()), block.getHash(), DBPrefix::heightToBlock);

  const auto& Txs = block.getTxs();

  for (uint32_t i = 0; i < Txs.size(); i++) {
    const auto& TxHash = Txs[i].hash();

    const FixedBytes<44> value(
      bytes::join(block.getHash(), Utils::uint32ToBytes(i), Utils::uint64ToBytes(block.getNHeight())));

    batch.push_back(TxHash, value, DBPrefix::txToBlock);
  }

  const std::string latestTag = "latest";
  const bytes::View latestTagBytes(reinterpret_cast<const Byte *>(latestTag.data()), latestTag.size());

  batch.push_back(latestTagBytes, block.getHash(), DBPrefix::tagToBlock);

  db.putBatch(batch);
}

Storage::Storage(std::string instanceIdStr, const Options& options)
  : instanceIdStr_(std::move(instanceIdStr)),
    db_(options.getRootPath() + "/blocksDb/"),
    options_(options)
{
  LOGINFO("Loading blockchain from DB");

  // Initialize the blockchain if latest block doesn't exist.
  initializeBlockchain();

  // Get the latest block from the database
  LOGINFO("Loading latest block");
  const Bytes latestBlockHash = db_.get(Utils::stringToBytes("latest"), DBPrefix::tagToBlock);

  if (latestBlockHash.empty())
    throw DynamicException("Latest block hash not found in DB");

  const Bytes latestBlockBytes = db_.get(latestBlockHash, DBPrefix::blocks);

  if (latestBlockBytes.empty())
    throw DynamicException("Latest block bytes not found in DB");

  latest_ = std::make_shared<const FinalizedBlock>(FinalizedBlock::fromBytes(latestBlockBytes, this->options_.getChainID()));

  LOGINFO("Latest block successfully loaded");
}

void Storage::initializeBlockchain() {
  // Genesis block comes from Options, not hardcoded
  const auto& genesis = options_.getGenesisBlock();

  if (!db_.has(std::string("latest"), DBPrefix::tagToBlock)) {
    if (genesis.getNHeight() != 0)
      throw DynamicException("Genesis block height is not 0");

    storeBlock(db_, genesis);

    const Hash blockHash = genesis.getHash();

    LOGINFO(std::string("Created genesis block: ") + Hex::fromBytes(genesis.getHash()).get());
  }

  // Sanity check for genesis block. (check if genesis in DB matches genesis in Options)
  const Hash genesisInDBHash(db_.get(Utils::uint64ToBytes(0), DBPrefix::heightToBlock));

  FinalizedBlock genesisInDB = FinalizedBlock::fromBytes(
    db_.get(genesisInDBHash, DBPrefix::blocks), options_.getChainID());

  if (genesis != genesisInDB) {
    LOGERROR("Sanity Check! Genesis block in DB does not match genesis block in Options");
    throw DynamicException("Sanity Check! Genesis block in DB does not match genesis block in Options");
  }
}

TxBlock Storage::getTxFromBlockWithIndex(bytes::View blockData, uint64_t txIndex) const {
  uint64_t index = 217; // Start of block tx range
  // Count txs until index.
  uint64_t currentTx = 0;
  while (currentTx < txIndex) {
    uint32_t txSize = Utils::bytesToUint32(blockData.subspan(index, 4));
    index += txSize + 4;
    currentTx++;
  }
  uint64_t txSize = Utils::bytesToUint32(blockData.subspan(index, 4));
  index += 4;
  return TxBlock(blockData.subspan(index, txSize), this->options_.getChainID());
}

void Storage::pushBlock(FinalizedBlock block) {
  auto previousBlock = latest_.load();

  if (previousBlock->getHash() != block.getPrevBlockHash())
    throw DynamicException("\"previous hash\" of new block does not match the latest block hash");

  auto newBlock = std::make_shared<FinalizedBlock>(std::move(block));
  latest_.store(newBlock);

  storeBlock(db_, *newBlock);
}

bool Storage::blockExists(const Hash& hash) const {
  return db_.has(hash, DBPrefix::blocks);
}

bool Storage::blockExists(uint64_t height) const {
  return db_.has(Utils::uint64ToBytes(height), DBPrefix::heightToBlock);
}

bool Storage::txExists(const Hash& tx) const {
  return db_.has(tx, DBPrefix::txToBlock);
}

std::shared_ptr<const FinalizedBlock> Storage::getBlock(const Hash& hash) const {
  Bytes blockBytes = db_.get(hash, DBPrefix::blocks);

  if (blockBytes.empty())
    return nullptr;

  return std::make_shared<FinalizedBlock>(FinalizedBlock::fromBytes(blockBytes, this->options_.getChainID()));
}

std::shared_ptr<const FinalizedBlock> Storage::getBlock(uint64_t height) const {
  Bytes blockHash = db_.get(Utils::uint64ToBytes(height), DBPrefix::heightToBlock);

  if (blockHash.empty())
    return nullptr;

  Bytes blockBytes = db_.get(blockHash, DBPrefix::blocks);

  // TODO: replace by getBlock(hash). for now it would be more expensive to do it so.s
  return std::make_shared<FinalizedBlock>(FinalizedBlock::fromBytes(blockBytes, this->options_.getChainID()));
}

std::tuple<
  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
  > Storage::getTx(const Hash& tx) const {
  const Bytes txData = db_.get(tx, DBPrefix::txToBlock);

  if (txData.empty())
    return std::make_tuple(nullptr, Hash(), 0u, 0u);

  const bytes::View txDataView = txData;
  const Hash blockHash(txDataView.subspan(0, 32));
  const uint64_t blockIndex = Utils::bytesToUint32(txDataView.subspan(32, 4));
  const uint64_t blockHeight = Utils::bytesToUint64(txDataView.subspan(36, 8));
  const Bytes blockData = db_.get(blockHash, DBPrefix::blocks);

  return std::make_tuple(std::make_shared<const TxBlock>(
    getTxFromBlockWithIndex(blockData, blockIndex)), blockHash, blockIndex, blockHeight);
}

std::tuple<
  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
  > Storage::getTxByBlockHashAndIndex(const Hash& blockHash, const uint64_t blockIndex) const {
  const Bytes blockData = db_.get(blockHash, DBPrefix::blocks);

  if (blockData.empty())
    std::make_tuple(nullptr, Hash(), 0u, 0u);

  const uint64_t blockHeight = Utils::bytesToUint64(bytes::View(blockData).subspan(201, 8));
  return std::make_tuple(std::make_shared<TxBlock>(
    getTxFromBlockWithIndex(blockData, blockIndex)), blockHash, blockIndex, blockHeight);
}

std::tuple<
  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
  > Storage::getTxByBlockNumberAndIndex(uint64_t blockHeight, uint64_t blockIndex) const {

  const Bytes blockHash = db_.get(Utils::uint64ToBytes(blockHeight), DBPrefix::heightToBlock);

  if (blockHash.empty())
    return std::make_tuple(nullptr, Hash(), 0u, 0u);
  
  const Bytes blockData = db_.get(blockHash, DBPrefix::blocks);

  if (blockData.empty())
    return std::make_tuple(nullptr, Hash(), 0u, 0u);

  return std::make_tuple(std::make_shared<TxBlock>(
    getTxFromBlockWithIndex(blockData, blockIndex)), Hash(blockHash), blockIndex, blockHeight);
}

std::shared_ptr<const FinalizedBlock> Storage::latest() const {
  return latest_.load();
}

uint64_t Storage::currentChainSize() const {
  auto latest = latest_.load();
  return latest ? (latest->getNHeight() + 1) : 0u;
}

void Storage::setContractAddress(const Hash& txHash, const Address& address) {
  db_.put(txHash, address, DBPrefix::txToAddr);
}

Address Storage::getContractAddress(const Hash& txHash) const {
  return Address(db_.get(txHash));
}

void Storage::setGasUsed(const Hash& txHash, const uint256_t& gasUsed) {
  db_.put(txHash, Utils::uintToBytes(gasUsed), DBPrefix::txToGasUsed);
}

uint256_t Storage::getGasUsed(const Hash& txHash) const {
  const Bytes gasUsedInBytes = db_.get(txHash, DBPrefix::txToGasUsed);
  return Utils::bytesToUint256(gasUsedInBytes);
}