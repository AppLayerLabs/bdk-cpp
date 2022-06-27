#include "state.h"

State::State(std::shared_ptr<DBService> &dbServer) {
  this->loadState(dbServer);
}

bool State::loadState(std::shared_ptr<DBService> &dbServer) {
  // Load chainHead
  stateLock.lock();
  // Load accounts.
  auto accounts = dbServer->readBatch(DBPrefix::nativeAccounts);
  for (auto account : accounts) {
    this->nativeAccount[account.key].balance = Utils::bytesToUint256(account.value.substr(0,32));
    this->nativeAccount[account.key].nonce = Utils::bytesToUint32(account.value.substr(32,32));
  }
  stateLock.unlock();
  return true;
}

bool State::saveState(std::shared_ptr<DBService> &dbServer) {
  // Save accounts.
  // Use DB batch methods.
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
  // Replay Protection.
  stateLock.lock();
  if (!tx.isReplayProtected()) {
    stateLock.unlock();
    return false;
  }

  // Check if sender nonce matches.
  if (this->nativeAccount[tx.hash()].nonce != tx.nonce()) {
    stateLock.unlock();
    return false;
  }

  // Check if the sender has enough balance.
  if (this->nativeAccount[tx.hash()].balance < tx.value()) {
    stateLock.unlock();
    return false;
  }

  // Check if already exists.
  if (this->mempool.count(tx.hash())) {
    stateLock.unlock();
    return false;
  }

  this->mempool[dev::toHex(dev::sha3(tx.rlp()))] = tx;
  stateLock.unlock();
  return true;
}

