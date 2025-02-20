#include "executioncontext.h"

void ExecutionContext::addEvent(Event event) {
  transactional::AnyTransactional emplaceTransaction(transactional::emplaceBack(events_, std::move(event)));
  transactions_.push(std::move(emplaceTransaction));
}

void ExecutionContext::addEvent(View<Address> address, View<Bytes> data, std::vector<Hash> topics) {
  this->addEvent(Event("", events_.size(), txHash_, txIndex_, blockHash_, blockNumber_, Address(address), Bytes(data), std::move(topics), !topics.empty()));
}


ExecutionContext::AccountPointer ExecutionContext::getAccount(View<Address> accountAddress) {
  return ExecutionContext::AccountPointer(*accounts_[accountAddress], transactions_);
}

BaseContract& ExecutionContext::getContract(View<Address> contractAddress) {
  const auto it = contracts_.find(contractAddress);

  if (it == contracts_.end()) {
    throw DynamicException("contract not found");
  }

  if (it->second == nullptr) {
    throw DynamicException("not a C++ contract");
  }

  return *it->second;
}

const BaseContract& ExecutionContext::getContract(View<Address> contractAddress) const {
  const auto it = contracts_.find(contractAddress);

  if (it == contracts_.end()) {
    throw DynamicException("contract not found");
  }

  if (it->second == nullptr) {
    throw DynamicException("not a C++ contract");
  }

  return *it->second;
}

Account& ExecutionContext::getMutableAccount(View<Address> accountAddress) {
  const auto iterator = accounts_.find(accountAddress);

  if (iterator == accounts_.end()) {
    throw DynamicException("account not found");
  }

  return *iterator->second;
}

bool ExecutionContext::accountExists(View<Address> accountAddress) const {
  return accounts_.contains(accountAddress);
}

void ExecutionContext::addContract(View<Address> address, std::unique_ptr<BaseContract> contract) {
  if (contract == nullptr) {
    throw DynamicException("attempt to insert null contract");
  }

  const auto [iterator, inserted] = contracts_.emplace(address, std::move(contract));

  if (!inserted) {
    throw DynamicException("contract already exists");
  }

  auto account = getAccount(address);
  account.setNonce(1);
  account.setContractType(ContractType::CPP);

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

void ExecutionContext::transferBalance(View<Address> fromAddress, View<Address> toAddress, const uint256_t& amount) {
  auto sender = getAccount(fromAddress);
  auto recipient = getAccount(toAddress);

  if (sender.getBalance() < amount) {
    throw DynamicException("insufficient founds");
  }

  sender.setBalance(sender.getBalance() - amount);
  recipient.setBalance(recipient.getBalance() + amount);
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

ExecutionContext::AccountPointer::AccountPointer(Account& account, std::stack<transactional::AnyTransactional>& transactions)
  : account_(account), transactions_(transactions) {}

const uint256_t& ExecutionContext::AccountPointer::getBalance() const{
  return account_.balance;
}

uint64_t ExecutionContext::AccountPointer::getNonce() const {
  return account_.nonce;
}

View<Hash> ExecutionContext::AccountPointer::getCodeHash() const {
  return account_.codeHash;
}

View<Bytes> ExecutionContext::AccountPointer::getCode() const {
  return account_.code;
}

ContractType ExecutionContext::AccountPointer::getContractType() const {
  return account_.contractType;
}

void ExecutionContext::AccountPointer::setBalance(const uint256_t& amount) {
  transactions_.push(transactional::AnyTransactional(transactional::copy(account_.balance)));
  account_.balance = amount;
}

void ExecutionContext::AccountPointer::setNonce(uint64_t nonce) {
  transactions_.push(transactional::AnyTransactional(transactional::copy(account_.nonce)));
  account_.nonce = nonce;
}

void ExecutionContext::AccountPointer::setCode(Bytes code) {
  transactions_.push(transactional::AnyTransactional(transactional::copy(account_.codeHash)));
  transactions_.push(transactional::AnyTransactional(transactional::copy(account_.code)));
  account_.codeHash = Utils::sha3(code);
  account_.code = std::move(code);
}

void ExecutionContext::AccountPointer::setContractType(ContractType type) {
  transactions_.push(transactional::AnyTransactional(transactional::copy(account_.contractType)));
  account_.contractType = type;
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
