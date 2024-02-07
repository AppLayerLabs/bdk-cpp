/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contractmanager.h"
#include "abi.h"
#include "contractfactory.h"
#include "contractcalllogger.h"
#include "customcontracts.h"
#include "../core/rdpos.h"
#include "../core/state.h"

ContractManager::ContractManager(
  const std::unique_ptr<DB>& db, State* state,
  const std::unique_ptr<rdPoS>& rdpos, const std::unique_ptr<Options>& options
) : BaseContract("ContractManager", ProtocolContractAddresses.at("ContractManager"), options->getChainOwner(), 0, db),
  state_(state),
  rdpos_(rdpos),
  options_(options),
  factory_(std::make_unique<ContractFactory>(*this)),
  interface_(std::make_unique<ContractManagerInterface>(*this)),
  eventManager_(std::make_unique<EventManager>(db, options))
{
  this->callLogger_ = std::make_unique<ContractCallLogger>(*this);
  this->factory_->registerContracts<ContractTypes>();
  this->factory_->addAllContractFuncs<ContractTypes>();
  // Load Contracts from DB
  std::vector<DBEntry> contractsFromDB = this->db_->getBatch(DBPrefix::contractManager);
  for (const DBEntry& contract : contractsFromDB) {
    Address address(contract.key);
    if (!this->loadFromDB<ContractTypes>(contract, address)) {
      throw std::runtime_error("Unknown contract: " + Utils::bytesToString(contract.value));
    }
  }
}

ContractManager::~ContractManager() {
  DBBatch contractsBatch;
  for (const auto& [address, contract] : this->contracts_) {
    contractsBatch.push_back(
      Bytes(address.asBytes()),
      Utils::stringToBytes(contract->getContractName()),
      DBPrefix::contractManager
    );
  }
  this->db_->putBatch(contractsBatch);
}

Address ContractManager::deriveContractAddress() const {
  // Contract address is last 20 bytes of sha3 ( rlp ( tx from address + tx nonce ) )
  uint8_t rlpSize = 0xc0;
    rlpSize += this->getCaller().size();
    // As we don't have actually access to the nonce, we will use the number of contracts existing in the chain
    rlpSize += (this->contracts_.size() < 0x80)
      ? 1 : 1 + Utils::bytesRequired(this->contracts_.size());
    Bytes rlp;
    rlp.insert(rlp.end(), rlpSize);
    rlp.insert(rlp.end(), this->getCaller().cbegin(), this->getCaller().cend());
    rlp.insert(rlp.end(), (this->contracts_.size() < 0x80)
      ? (char)this->contracts_.size()
      : (char)0x80 + Utils::bytesRequired(this->contracts_.size())
  );
  return Address(Utils::sha3(rlp).view_const(12));
}

Bytes ContractManager::getDeployedContracts() const {
  std::shared_lock<std::shared_mutex> lock(this->contractsMutex_);
  std::vector<std::string> names;
  std::vector<Address> addresses;
  for (const auto& [address, contract] : this->contracts_) {
    names.push_back(contract->getContractName());
    addresses.push_back(address);
  }
  Bytes result = ABI::Encoder::encodeData(names, addresses);
  return result;
}

void ContractManager::ethCall(const ethCallInfo& callInfo) {
  Functor functor = std::get<5>(callInfo);
  std::function<void(const ethCallInfo&)> f;
  f = this->factory_->getCreateContractFunc(functor.asBytes());
  if (!f) throw std::runtime_error("Invalid function call with functor: " + functor.hex().get());
  f(callInfo);
}

const Bytes ContractManager::ethCallView(const ethCallInfo& data) const {
  const auto& functor = std::get<5>(data);
  // This hash is equivalent to "function getDeployedContracts() public view returns (string[] memory, address[] memory) {}"
  if (functor == Hex::toBytes("0xaa9a068f")) return this->getDeployedContracts();
  throw std::runtime_error("Invalid function call");
}

void ContractManager::callContract(const TxBlock& tx, const Hash&, const uint64_t& txIndex) {
  this->callLogger_ = std::make_unique<ContractCallLogger>(*this);
  auto callInfo = tx.txToCallInfo();
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  if (to == this->getContractAddress()) {
    this->callLogger_->setContractVars(this, from, from, value);
    try {
      this->ethCall(callInfo);
    } catch (std::exception &e) {
      this->callLogger_.reset();
      this->eventManager_->revertEvents();
      throw std::runtime_error(e.what());
    }
    this->callLogger_->shouldCommit();
    this->callLogger_.reset();
    this->eventManager_->commitEvents(tx.hash(), txIndex);
    return;
  }

  if (to == ProtocolContractAddresses.at("rdPoS")) {
    this->callLogger_->setContractVars(rdpos_.get(), from, from, value);
    try {
      rdpos_->ethCall(callInfo);
    } catch (std::exception &e) {
      this->callLogger_.reset();
      this->eventManager_->revertEvents();
      throw std::runtime_error(e.what());
    }
    this->callLogger_->shouldCommit();
    this->callLogger_.reset();
    this->eventManager_->commitEvents(tx.hash(), txIndex);
    return;
  }

  std::unique_lock lock(this->contractsMutex_);
  auto it = this->contracts_.find(to);
  if (it == this->contracts_.end()) {
    this->callLogger_.reset();
    this->eventManager_->revertEvents();
    throw std::runtime_error(std::string(__func__) + "(void): Contract does not exist");
  }

  const std::unique_ptr<DynamicContract>& contract = it->second;
  this->callLogger_->setContractVars(contract.get(), from, from, value);
  try {
    contract->ethCall(callInfo);
  } catch (std::exception &e) {
    this->callLogger_.reset();
    this->eventManager_->revertEvents();
    throw std::runtime_error(e.what());
  }

  if (contract->isPayableFunction(functor)) {
    this->state_->processContractPayable(this->callLogger_->getBalances());
  }
  this->callLogger_->shouldCommit();
  this->callLogger_.reset();
  this->eventManager_->commitEvents(tx.hash(), txIndex);
}

const Bytes ContractManager::callContract(const ethCallInfo& callInfo) const {
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  if (to == this->getContractAddress()) return this->ethCallView(callInfo);
  if (to == ProtocolContractAddresses.at("rdPoS")) return rdpos_->ethCallView(callInfo);
  std::shared_lock<std::shared_mutex> lock(this->contractsMutex_);
  if (!this->contracts_.contains(to)) {
    throw std::runtime_error(std::string(__func__) + "(Bytes): Contract does not exist");
  }
  return this->contracts_.at(to)->ethCallView(callInfo);
}

bool ContractManager::isPayable(const ethCallInfo& callInfo) const {
  const auto& address = std::get<1>(callInfo);
  const auto& functor = std::get<5>(callInfo);
  std::shared_lock<std::shared_mutex> lock(this->contractsMutex_);
  auto it = this->contracts_.find(address);
  if (it == this->contracts_.end()) return false;
  return it->second->isPayableFunction(functor);
}

bool ContractManager::validateCallContractWithTx(const ethCallInfo& callInfo) {
  this->callLogger_ = std::make_unique<ContractCallLogger>(*this);
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  try {
    if (value) {
      // Payable, we need to "add" the balance to the contract
      this->interface_->populateBalance(from);
      this->interface_->populateBalance(to);
      this->callLogger_->subBalance(from, value);
      this->callLogger_->addBalance(to, value);
    }
    if (to == this->getContractAddress()) {
      this->callLogger_->setContractVars(this, from, from, value);
      this->ethCall(callInfo);
      this->callLogger_.reset();
      return true;
    }

    if (to == ProtocolContractAddresses.at("rdPoS")) {
      this->callLogger_->setContractVars(rdpos_.get(), from, from, value);
      rdpos_->ethCall(callInfo);
      this->callLogger_.reset();
      return true;
    }

    std::shared_lock<std::shared_mutex> lock(this->contractsMutex_);
    if (!this->contracts_.contains(to)) {
      this->callLogger_.reset();
      return false;
    }
    const auto &contract = contracts_.at(to);
    this->callLogger_->setContractVars(contract.get(), from, from, value);
    contract->ethCall(callInfo);
  } catch (std::exception &e) {
    this->callLogger_.reset();
    throw std::runtime_error(e.what());
  }
  this->callLogger_.reset();
  return true;
}

bool ContractManager::isContractCall(const TxBlock &tx) const {
  if (tx.getTo() == this->getContractAddress()) return true;
  for (const auto& [protocolName, protocolAddress] : ProtocolContractAddresses) {
    if (tx.getTo() == protocolAddress) return true;
  }
  std::shared_lock<std::shared_mutex> lock(this->contractsMutex_);
  return this->contracts_.contains(tx.getTo());
}

bool ContractManager::isContractAddress(const Address &address) const {
  std::shared_lock<std::shared_mutex> lock(this->contractsMutex_);
  for (const auto& [protocolName, protocolAddress] : ProtocolContractAddresses) {
    if (address == protocolAddress) return true;
  }
  return this->contracts_.contains(address);
}

std::vector<std::pair<std::string, Address>> ContractManager::getContracts() const {
  std::shared_lock<std::shared_mutex> lock(this->contractsMutex_);
  std::vector<std::pair<std::string, Address>> contracts;
  for (const auto& [address, contract] : this->contracts_) {
    contracts.emplace_back(std::make_pair(contract->getContractName(), address));
  }
  return contracts;
}

const std::vector<Event> ContractManager::getEvents(
  const uint64_t& fromBlock, const uint64_t& toBlock,
  const Address& address, const std::vector<Hash>& topics
) const {
  return this->eventManager_->getEvents(fromBlock, toBlock, address, topics);
}

const std::vector<Event> ContractManager::getEvents(
  const Hash& txHash, const uint64_t& blockIndex, const uint64_t& txIndex
) const {
  return this->eventManager_->getEvents(txHash, blockIndex, txIndex);
}

void ContractManager::updateContractGlobals(
  const Address& coinbase, const Hash& blockHash,
  const uint64_t& blockHeight, const uint64_t& blockTimestamp
) const {
  ContractGlobals::coinbase_ = coinbase;
  ContractGlobals::blockHash_ = blockHash;
  ContractGlobals::blockHeight_ = blockHeight;
  ContractGlobals::blockTimestamp_ = blockTimestamp;
}

void ContractManagerInterface::registerVariableUse(SafeBase& variable) {
  this->manager_.callLogger_->addUsedVar(variable);
}

void ContractManagerInterface::populateBalance(const Address &address) const {
  if (!this->manager_.callLogger_) throw std::runtime_error(
    "Contracts going haywire! Trying to call ContractState without an active callContract"
  );
  if (!this->manager_.callLogger_->hasBalance(address)) {
    auto it = this->manager_.state_->accounts_.find(address);
    this->manager_.callLogger_->setBalanceAt(address,
      (it != this->manager_.state_->accounts_.end()) ? it->second.balance : 0
    );
  }
}

uint256_t ContractManagerInterface::getBalanceFromAddress(const Address& address) const {
  if (!this->manager_.callLogger_) throw std::runtime_error(
    "Contracts going haywire! Trying to call ContractState without an active callContract"
  );
  this->populateBalance(address);
  return this->manager_.callLogger_->getBalanceAt(address);
}

void ContractManagerInterface::sendTokens(
  const Address& from, const Address& to, const uint256_t& amount
) {
  if (!this->manager_.callLogger_) throw std::runtime_error(
    "Contracts going haywire! Trying to call ContractState without an active callContract"
  );
  this->populateBalance(from);
  this->populateBalance(to);
  if (this->manager_.callLogger_->getBalanceAt(from) < amount) {
    throw std::runtime_error("ContractManager::sendTokens: Not enough balance");
  }
  this->manager_.callLogger_->subBalance(from, amount);
  this->manager_.callLogger_->addBalance(to, amount);
}

