#include "state.h"

State::State(std::shared_ptr<DBService> &dbServer) {
  this->loadState(dbServer);
}

bool State::loadState(std::shared_ptr<DBService> &dbServer) {
  stateLock.lock();
  auto accounts = dbServer->readBatch(DBPrefix::nativeAccounts);
  for (auto account : accounts) {
    this->nativeAccount[account.key].balance = Utils::bytesToUint256(account.value.substr(0,32));
    this->nativeAccount[account.key].nonce = Utils::bytesToUint32(account.value.substr(32,32));
  }
  stateLock.unlock();
  return true;
}

bool State::saveState(std::shared_ptr<DBService> &dbServer) {
  stateLock.lock();
  WriteBatchRequest accountsBatch;
  for (auto &account : this->nativeAccount) {
    accountsBatch.puts.emplace_back(
      account.first,
      Utils::uint256ToBytes(account.second.balance) + Utils::uint32ToBytes(account.second.nonce)
    );
  }
  dbServer->writeBatch(accountsBatch, DBPrefix::nativeAccounts);
  stateLock.unlock();
  return true;
}

bool State::validateTransaction(dev::eth::TransactionBase& tx) {
  // TODO: Handle error conditions to report at RPC level.
  // TODO: Handle transaction override (if the transaction is already in the mempool).
  // TODO: Handle transaction queue for multiple tx's from single user.
  stateLock.lock();

  // Replay protection.
  if (!tx.isReplayProtected()) {
    stateLock.unlock();
    return false;
  }

  // Check if sender nonce matches.
  if (this->nativeAccount[tx.hash()].nonce != tx.nonce()) {
    stateLock.unlock();
    return false;
  }

  // Check if sender has enough balance.
  if (this->nativeAccount[tx.hash()].balance < tx.value()) {
    stateLock.unlock();
    return false;
  }

  // Check if transaction already exists in mempool.
  if (this->mempool.count(tx.hash())) {
    stateLock.unlock();
    return false;
  }

  this->mempool[tx.hash()] = tx;
  stateLock.unlock();
  return true;
}

bool State::processNewTransaction(const dev::eth::TransactionBase& tx) {
  bool isContractCall = false;

  // Remove transaction from mempool if found there.
  if (this->mempool.count(tx.hash()) != 0) this->mempool.erase(tx.hash());

  // Update Balances.
  this->nativeAccount[dev::toHex(tx.from())].balance -= tx.value();
  this->nativeAccount[dev::toHex(tx.to())].balance += tx.value();

  // Update nonce.
  this->nativeAccount[dev::toHex(tx.from())].nonce++;

  // Burn gas fees.
  this->nativeAccount[dev::toHex(tx.from())].balance -= (tx.gasPrice() * tx.gas());

  return true;
}

bool State::processNewBlock(Block& newBlock, std::unique_ptr<ChainHead>& chainHead) {
  stateLock.lock();

  // Check block previous hash.
  Block bestBlock = chainHead->latest();
  if (bestBlock.getBlockHash() != newBlock.prevBlockHash()) {
    stateLock.unlock();
    Utils::LogPrint(Log::state, __func__, "Block previous hash does not match.");
    Utils::LogPrint(Log::state, __func__, "newBlock previous hash: " + newBlock.prevBlockHash());
    Utils::LogPrint(Log::state, __func__, "bestBlock hash: " + bestBlock.getBlockHash());
    return false;
  }

  if (newBlock.nHeight() != (bestBlock.nHeight() + 1)) {
    stateLock.unlock();
    Utils::LogPrint(Log::state, __func__, "Block height does not match.");
    Utils::LogPrint(Log::state, __func__, "newBlock height: " + std::to_string(newBlock.nHeight()));
    Utils::LogPrint(Log::state, __func__, "bestBlock height: " + std::to_string(bestBlock.nHeight()));
    return false;
  }

  // TODO: Check creator.

  for (auto &tx : newBlock.transactions()) {
    this->processNewTransaction(tx);
  }

  // Append block to chainHead.
  chainHead->push_back(newBlock);

  stateLock.unlock();
  return true;
}

bool State::createNewBlock(std::unique_ptr<ChainHead>& chainHead) {
  stateLock.lock();
  Block bestBlock = chainHead->latest();
  // TODO: Is this cast wasting memory?
  Block newBestBlock(
    Utils::bytesToUint256(bestBlock.getBlockHash()),
    // the block cast one one the time looool.
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(),
    bestBlock.nHeight() + 1
  );
  // Lock state to load transactions from mempool

  for (auto &tx : this->mempool) {
    newBestBlock.appendTx(tx.second);
  }

  newBestBlock.finalizeBlock();
  // Unlock mutex as processNewBlock will lock it again.
  stateLock.unlock();
  return this->processNewBlock(newBestBlock, chainHead);
}

uint256_t State::getNativeBalance(const std::string& address) {
  uint256_t ret;
  this->stateLock.lock();
  ret = this->nativeAccount[address].balance;
  this->stateLock.unlock();
  return ret;
};

uint256_t State::getNativeNonce(const std::string& address) {
  uint256_t ret;
  this->stateLock.lock();
  ret = this->nativeAccount[address].nonce;
  this->stateLock.unlock();
  return ret;
};

void State::addBalance(std::string &address) {
  this->stateLock.lock();
  this->nativeAccount[address].balance += 1000000000000000000;
  this->stateLock.unlock();
}

