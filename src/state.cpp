#include "state.h"


// latest (the latest block) needs to be constructed with the State constructor.
State::State(std::shared_ptr<DBService> &dbServer) : latest(dbServer->get("latest", DBPrefix::blocks)) {
  this->loadState(dbServer);
}

bool State::loadState(std::shared_ptr<DBService> &dbServer) {
  // Load accounts.
  auto accounts = dbServer->readBatch(DBPrefix::nativeAccounts);
  for (auto account : accounts) {
    this->nativeAccount[account.key].balance = Utils::bytesToUint256(account.value.substr(0,32));
    this->nativeAccount[account.key].nonce = Utils::bytesToUint32(account.value.substr(32,32));
  }
  return true;
}

bool State::saveState(std::shared_ptr<DBService> &dbServer) {
  // Save accounts.
  // Use DB batch methods.
  WriteBatchRequest accountsBatch;
  for (auto &account : this->nativeAccount) {
    accountsBatch.puts.emplace_back(
      account.first,
      Utils::uint256ToBytes(account.second.balance) + Utils::uint32ToBytes(account.second.nonce)
    );
  }
  dbServer->writeBatch(accountsBatch, DBPrefix::nativeAccounts);

  return true;
}