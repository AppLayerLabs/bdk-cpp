#include "executioncontext.h"

void ExecutionContext::addEvent(Event event) {
  transactional::AnyTransactional emplaceTransaction(transactional::emplaceBack(events_, std::move(event)));
  transactions_.push(std::move(emplaceTransaction));
}

void ExecutionContext::addEvent(View<Address> address, View<Bytes> data, std::vector<Hash> topics) {
  this->addEvent(Event("", events_.size(), txHash_, txIndex_, blockHash_, blockNumber_, Address(address), Bytes(data), std::move(topics), !topics.empty()));
}


const Account& ExecutionContext::getAccount(View<Address> accountAddress) const {
  const auto iterator = accounts_.find(accountAddress);

  if (iterator == accounts_.end()) {
    throw DynamicException("account not found"); // TODO: dynamic exception
  }

  return *iterator->second;
}

BaseContract& ExecutionContext::getContract(View<Address> contractAddress) {
  const auto it = contracts_.find(contractAddress);

  if (it == contracts_.end()) {
    throw DynamicException("contract not found"); // TODO: dynamic exception
  }

  if (it->second == nullptr) {
    throw DynamicException("not a C++ contract"); // TODO: dynamic exception
  }

  return *it->second;
}

const BaseContract& ExecutionContext::getContract(View<Address> contractAddress) const {
  const auto it = contracts_.find(contractAddress);

  if (it == contracts_.end()) {
    throw DynamicException("contract not found"); // TODO: dynamic exception
  }

  if (it->second == nullptr) {
    throw DynamicException("not a C++ contract"); // TODO: dynamic exception
  }

  return *it->second;
}

Account& ExecutionContext::getMutableAccount(View<Address> accountAddress) {
  const auto iterator = accounts_.find(accountAddress);

  if (iterator == accounts_.end()) {
    throw DynamicException("account not found"); // TODO: dynamic exception
  }

  return *iterator->second;
}

bool ExecutionContext::accountExists(View<Address> accountAddress) const {
  return accounts_.contains(accountAddress);
}

void ExecutionContext::addAccount(View<Address> address, Account account) {
  auto [transaction, inserted] = transactional::emplace(accounts_, address, std::move(account));

  if (!inserted) {
    throw DynamicException("account already exist"); // TODO: dynamic exception
  }

  transactions_.push(transactional::AnyTransactional(std::move(transaction)));
}

void ExecutionContext::addContract(View<Address> address, std::unique_ptr<BaseContract> contract) {
  const auto [iterator, inserted] = contracts_.emplace(address, std::move(contract));

  if (!inserted) {
    throw DynamicException("contract already exists");
  }

  addAccount(address, Account::makeCppContract());
  notifyNewContract(iterator->first, iterator->second.get());
}

void ExecutionContext::notifyNewContract(View<Address> address, BaseContract* contract) {
  using ContractsTuple = std::tuple<ExecutionContext::Contracts&, std::vector<std::pair<Address, BaseContract*>>&>;

  newContracts_.emplace_back(address, contract);

  transactions_.push(transactional::AnyTransactional(transactional::BasicTransactional(contracts_, [contractAddress = Address(address)] (auto& contracts) {
    contracts.erase(contractAddress);
  })));

  transactions_.push(transactional::AnyTransactional(transactional::BasicTransactional(newContracts_, [] (auto& newContracts) {
    newContracts.pop_back();
  })));
}

void addContract(View<Address> address, BaseContract* contract) {
  // TODO: ???
  // TODO: take as std::unique_ptr and save the contract?
  // TODO: analyse ContractHost and see what is done when a C++ contract is created
  // TODO: remember that this function must also be called for EVM contracts (nullptr in this case)
}

void ExecutionContext::transferBalance(View<Address> fromAddress, View<Address> toAddress, const uint256_t& amount) {
  Account& sender = getMutableAccount(fromAddress);
  Account& recipient = getMutableAccount(toAddress);

  if (sender.balance < amount) {
    throw DynamicException("insufficient founds");
  }

  transactional::AnyTransactional senderTransaction(transactional::copy(sender.balance));
  transactional::AnyTransactional recipientTransaction(transactional::copy(recipient.balance));

  sender.balance -= amount;
  recipient.balance += amount;

  transactions_.push(std::move(senderTransaction));
  transactions_.push(std::move(recipientTransaction));
}

void ExecutionContext::incrementNonce(View<Address> accountAddress) {
  Account& account = getMutableAccount(accountAddress);
  transactions_.push(transactional::AnyTransactional(transactional::copy(account.nonce)));
  ++account.nonce;
}

void ExecutionContext::store(View<Address> addr, View<Hash> slot, View<Hash> data) {
  transactional::AnyTransactional transaction(transactional::emplaceOrAssign(storage_, StorageKeyView(addr, slot), data));
  transactions_.push(std::move(transaction));
}

Hash ExecutionContext::retrieve(View<Address> addr, View<Hash> slot) const {
  const auto iterator = storage_.find(StorageKeyView(addr, slot));
  return (iterator == storage_.end()) ? Hash() : iterator->second;
}

void ExecutionContext::commit() {  
  while (!transactions_.empty()) {
    transactions_.top().commit();
    transactions_.pop();
  }

  events_.clear();
  newContracts_.clear();
}

void ExecutionContext::revert() {
  while (!transactions_.empty()) {
    transactions_.pop(); // transactions revert on destructor (by default)
  }

  events_.clear();
  newContracts_.clear();
}

ExecutionContext::Checkpoint ExecutionContext::checkpoint() {
  return ExecutionContext::Checkpoint(transactions_);
}

ExecutionContext::Checkpoint::Checkpoint(std::stack<transactional::AnyTransactional>& transactions)
  : transactions_(&transactions), checkpoint_(transactions.size()) {}

void ExecutionContext::Checkpoint::commit() {
  transactions_ = nullptr;
}

void ExecutionContext::Checkpoint::revert() {
  if (transactions_ == nullptr)
    return;
  
  while (transactions_->size() > checkpoint_)
    transactions_->pop();

  transactions_ = nullptr;
}
