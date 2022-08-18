#include "state.h"

#if !IS_LOCAL_TESTS
State::State(std::shared_ptr<DBService> &dbServer, std::shared_ptr<VMCommClient> &grpcClient) : grpcClient(grpcClient) {
  this->loadState(dbServer);
}
#else
State::State(std::shared_ptr<DBService> &dbServer) {
  this->loadState(dbServer);
}
#endif

bool State::loadState(std::shared_ptr<DBService> &dbServer) {
  stateLock.lock();
  auto accounts = dbServer->readBatch(DBPrefix::nativeAccounts);

  if (accounts.size() == 0) {
    Address ("0x21B782f9BF82418A42d034517CB6Bf00b4C17612", true);
    dbServer->put(, )
    stateLock.unlock();
    return false;
  }
  for (auto account : accounts) {
    Address address(account.key, false);
    this->nativeAccount[address].balance = Utils::bytesToUint256(account.value.substr(0,32));
    this->nativeAccount[address].nonce = Utils::bytesToUint32(account.value.substr(32,4));
  }
  stateLock.unlock();
  return true;
}

bool State::saveState(std::shared_ptr<DBService> &dbServer) {
  stateLock.lock();
  WriteBatchRequest accountsBatch;
  for (auto &account : this->nativeAccount) {
    accountsBatch.puts.emplace_back(
      account.first.get(),
      Utils::uint256ToBytes(account.second.balance) + Utils::uint32ToBytes(account.second.nonce)
    );
  }
  dbServer->writeBatch(accountsBatch, DBPrefix::nativeAccounts);
  stateLock.unlock();
  return true;
}

std::pair<int, std::string> State::validateTransaction(Tx::Base& tx) {
  // TODO: Handle error conditions to report at RPC level:
  // https://www.jsonrpc.org/specification#error_object
  // https://eips.ethereum.org/EIPS/eip-1474#error-codes
  // TODO: Handle transaction queue for multiple tx's from single user.
  stateLock.lock();

  int err = 0;
  std::string errMsg = "";
  if (this->nativeAccount[tx.from()].nonce != tx.nonce()) {
    err = -32003; errMsg = "Nonce mismatch";
  } else if (this->nativeAccount[tx.from()].balance < tx.value()) {
    err = -32003; errMsg = "Insufficient balance - required: " +
      boost::lexical_cast<std::string>(tx.value()) + " available: " +
      boost::lexical_cast<std::string>(this->nativeAccount[tx.from()].balance);
  } else if (this->mempool.count(tx.hash())) {
    err = -32003; errMsg = "Transaction already exists in mempool";
  }

  if (err != 0) {
    errMsg.insert(0, "Transaction rejected: ");
    Utils::LogPrint(Log::subnet, "validateTransaction", errMsg);
  } else {
    this->mempool[tx.hash()] = tx;
    #if !IS_LOCAL_TESTS
      grpcClient->requestBlock();
    #endif
  }

  stateLock.unlock();
  return std::make_pair(err, errMsg);
}

bool State::processNewTransaction(const Tx::Base& tx) {
  bool isContractCall = false;

  // TODO: Check Balance.

  // Remove transaction from mempool if found there.
  if (this->mempool.count(tx.hash()) != 0) this->mempool.erase(tx.hash());

  // Update Balances.
  this->nativeAccount[tx.from()].balance -= tx.value();
  this->nativeAccount[tx.to()].balance += tx.value();

  // Update nonce.
  this->nativeAccount[tx.from()].nonce++;

  // Burn gas fees.
  this->nativeAccount[tx.from()].balance -= (tx.gasPrice() * tx.gas());

  // TODO: Handle contract calls.

  return true;
}

bool State::processNewBlock(Block& newBlock, std::unique_ptr<ChainHead>& chainHead) {
  stateLock.lock();
  // Check block previous hash.
  auto bestBlock = chainHead->latest();
  if (bestBlock->getBlockHash() != newBlock.prevBlockHash()) {
    stateLock.unlock();
    Utils::LogPrint(Log::state, __func__, "Block previous hash does not match.");
    Utils::LogPrint(Log::state, __func__, "newBlock previous hash: " + newBlock.prevBlockHash());
    Utils::LogPrint(Log::state, __func__, "bestBlock hash: " + bestBlock->getBlockHash());
    return false;
  }
  if (newBlock.nHeight() != (1 + bestBlock->nHeight())) {
    stateLock.unlock();
    Utils::LogPrint(Log::state, __func__, "Block height does not match.");
    Utils::LogPrint(Log::state, __func__, "newBlock height: " + std::to_string(newBlock.nHeight()));
    Utils::LogPrint(Log::state, __func__, "bestBlock height: " + std::to_string(bestBlock->nHeight()));
    return false;
  }
  for (auto &tx : newBlock.transactions()) {
    this->processNewTransaction(tx);
  }

  // When the block is included in the chain, the transactions are indexed.
  newBlock.indexTxs();
  // Append block to chainHead.
  Utils::LogPrint(Log::state, __func__, "Appending block to chainHead.");
  chainHead->push_back(newBlock);
  Utils::LogPrint(Log::state, __func__, "Appended.");
  stateLock.unlock();
  return true;
}

bool State::createNewBlock(std::unique_ptr<ChainHead>& chainHead) {
  stateLock.lock();
  auto bestBlock = chainHead->latest();
  Block newBestBlock(
    Utils::bytesToUint256(bestBlock->getBlockHash()),
    std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count(),
    bestBlock->nHeight() + 1
  );
  for (auto &tx : this->mempool) newBestBlock.appendTx(tx.second);
  newBestBlock.finalizeBlock();
  stateLock.unlock(); // processNewBlock() will lock it again
  return this->processNewBlock(newBestBlock, chainHead);
}

uint256_t State::getNativeBalance(const Address& address) {
  uint256_t ret;
  this->stateLock.lock();
  ret = this->nativeAccount[address].balance;
  this->stateLock.unlock();
  return ret;
};

uint256_t State::getNativeNonce(const Address& address) {
  uint256_t ret;
  this->stateLock.lock();
  ret = this->nativeAccount[address].nonce;
  this->stateLock.unlock();
  return ret;
};

void State::addBalance(Address &address) {
  this->stateLock.lock();
  this->nativeAccount[address].balance += 1000000000000000000;
  this->stateLock.unlock();
}

