#include "contractmanager.h"
#include "contractfactory.h"
#include "customcontracts.h"
#include "../core/rdpos.h"
#include "../core/state.h"

ContractManager::ContractManager(
  State* state, const std::unique_ptr<DB>& db,
  const std::unique_ptr<rdPoS>& rdpos, const std::unique_ptr<Options>& options
) : state(state), BaseContract("ContractManager", ProtocolContractAddresses.at("ContractManager"),
  Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), 0, db),
  rdpos(rdpos),
  options(options),
  factory(std::make_unique<ContractFactory>(*this)),
  callState(std::make_unique<ContractCallState>()),
  interface(std::make_unique<ContractManagerInterface>(*this))
{
  this->factory->registerContracts<ContractTypes>();
  this->factory->addAllContractFuncs<ContractTypes>();
  // Load Contracts from DB
  std::vector<DBEntry> contractsFromDB = this->db->getBatch(DBPrefix::contractManager);
  for (const DBEntry& contract : contractsFromDB) {
    Address address(contract.key);
    if (!this->loadFromDB<ContractTypes>(contract, address)) {
      throw std::runtime_error("Unknown contract: " + Utils::bytesToString(contract.value));
    }
  }
  this->updateState(true);
}

ContractManager::~ContractManager() {
  DBBatch contractsBatch;
  for (const auto& [address, contract] : this->contracts) {
    contractsBatch.push_back(
      Bytes(address.asBytes()),
      Utils::stringToBytes(contract->getContractName()),
      DBPrefix::contractManager
    );
  }
  this->db->putBatch(contractsBatch);
}

void ContractManager::updateState(const bool commitToState) {
  if (commitToState) {
    this->callState->commitUsedVars();
  } else {
    this->callState->revertUsedVars();
    for (const Address& badContract : this->factory->getRecentContracts()) {
      this->contracts.erase(badContract); // Erase failed contract creations
    }
  }
  this->factory->clearRecentContracts();
  this->callState->clearUsedVars();
}

Address ContractManager::deriveContractAddress() const {
  // Contract address = sha3(rlp(tx.from() + tx.nonce()).substr(12);
  uint8_t rlpSize = 0xc0;
    rlpSize += this->getCaller().size();
    // As we don't have actually access to the nonce, we will use the number of contracts existing in the chain
    rlpSize += (this->contracts.size() < 0x80)
      ? 1 : 1 + Utils::bytesRequired(this->contracts.size());
    Bytes rlp;
    rlp.insert(rlp.end(), rlpSize);
    rlp.insert(rlp.end(), this->getCaller().cbegin(), this->getCaller().cend());
    rlp.insert(rlp.end(), (this->contracts.size() < 0x80)
      ? (char)this->contracts.size()
      : (char)0x80 + Utils::bytesRequired(this->contracts.size())
  );
  return Address(Utils::sha3(rlp).view_const(12));
}

Bytes ContractManager::getDeployedContracts() const {
  std::unique_lock lock(this->contractsMutex);
  std::vector<std::string> names;
  std::vector<Address> addresses;
  for (const auto& [address, contract] : this->contracts) {
    names.push_back(contract->getContractName());
    addresses.push_back(address);
  }
  ABI::Encoder::EncVar vars;
  vars.push_back(names);
  vars.push_back(addresses);
  return ABI::Encoder(vars).getData();
}

void ContractManager::ethCall(const ethCallInfo& callInfo) {
  Functor functor = std::get<5>(callInfo);
  std::function<void(const ethCallInfo&)> f;
  f = this->factory->getCreateContractFunc(functor.asBytes());
  if (!f) throw std::runtime_error("Invalid function call with functor: " + functor.hex().get());
  f(callInfo);
}

const Bytes ContractManager::ethCallView(const ethCallInfo& data) const {
  const auto& functor = std::get<5>(data);
  // function getDeployedContracts() public view returns (string[] memory, address[] memory) {}
  if (functor == Hex::toBytes("0xaa9a068f")) return this->getDeployedContracts();
  throw std::runtime_error("Invalid function call");
}

void ContractManager::callContract(const TxBlock& tx) {
  auto callInfo = tx.txToCallInfo();
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  if (to == this->getContractAddress()) {
    this->commit = true;
    this->caller = from;
    this->origin = from;
    this->value = value;
    try {
      this->ethCall(callInfo);
    } catch (std::exception &e) {
      this->commit = false;
      this->updateState(false);
      this->callState->clearBalances();
      throw std::runtime_error(e.what());
    }
    this->updateState(true);
    this->commit = false;
    this->callState->clearBalances();
    return;
  }

  if (to == ProtocolContractAddresses.at("rdPoS")) {
    rdpos->caller = from;
    rdpos->origin = from;
    rdpos->value = value;
    rdpos->commit = true;
    try {
      rdpos->ethCall(callInfo);
    } catch (std::exception &e) {
      rdpos->commit = false;
      this->updateState(false);
      this->callState->clearBalances();
      throw std::runtime_error(e.what());
    }
    rdpos->commit = false;
    this->updateState(true);
    this->callState->clearBalances();
    return;
  }

  std::unique_lock lock(this->contractsMutex);
  if (!this->contracts.contains(to)) {
    this->callState->clearBalances();
    throw std::runtime_error("Contract does not exist");
  }

  const auto& contract = contracts.at(to);
  contract->caller = from;
  contract->origin = from;
  contract->value = value;
  contract->commit = true;
  try {
    contract->ethCall(callInfo);
  } catch (std::exception &e) {
    contract->commit = false;
    this->updateState(false);
    this->callState->clearBalances();
    throw std::runtime_error(e.what());
  }

  if (contract->isPayableFunction(functor)) {
    this->state->processContractPayable(this->callState->getBalances());
  }
  this->callState->clearBalances();
  this->updateState(true);
  contract->commit = false;
}

const Bytes ContractManager::callContract(const ethCallInfo& callInfo) const {
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  if (to == this->getContractAddress()) return this->ethCallView(callInfo);
  if (to == ProtocolContractAddresses.at("rdPoS")) return rdpos->ethCallView(callInfo);
  std::shared_lock lock(this->contractsMutex);
  if (!this->contracts.contains(to)) throw std::runtime_error("Contract does not exist");
  return this->contracts.at(to)->ethCallView(callInfo);
}

bool ContractManager::isPayable(const ethCallInfo& callInfo) const {
  const auto& address = std::get<1>(callInfo);
  const auto& functor = std::get<5>(callInfo);
  std::shared_lock lock(this->contractsMutex);
  auto it = this->contracts.find(address);
  if (it == this->contracts.end()) return false;
  return it->second->isPayableFunction(functor);
}

bool ContractManager::validateCallContractWithTx(const ethCallInfo& callInfo) {
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  try {
    if (this->getValue()) {
      // Payable, we need to "add" the balance to the contract
      this->interface->populateBalance(to);
      this->callState->addBalance(to, value);
    }
    if (to == this->getContractAddress()) {
      this->caller = from;
      this->origin = from;
      this->value = value;
      this->ethCall(callInfo);
      this->callState->clearBalances();
      this->updateState(false);
      return true;
    }

    if (to == ProtocolContractAddresses.at("rdPoS")) {
      rdpos->caller = from;
      rdpos->origin = from;
      rdpos->value = value;
      rdpos->commit = false;
      rdpos->ethCall(callInfo);
      this->callState->clearBalances();
      this->updateState(false);
      return true;
    }

    std::shared_lock lock(this->contractsMutex);
    if (!this->contracts.contains(to)) {
      this->callState->clearBalances();
      return false;
    }
    const auto &contract = contracts.at(to);
    contract->caller = from;
    contract->origin = from;
    contract->value = value;
    contract->commit = false;
    contract->ethCall(callInfo);
  } catch (std::exception &e) {
    this->callState->clearBalances();
    this->updateState(false);
    throw std::runtime_error(e.what());
  }
  this->updateState(false);
  this->callState->clearBalances();
  return true;
}

bool ContractManager::isContractCall(const TxBlock &tx) const {
  if (tx.getTo() == this->getContractAddress()) return true;
  for (const auto& [protocolName, protocolAddress] : ProtocolContractAddresses) {
    if (tx.getTo() == protocolAddress) return true;
  }
  std::shared_lock lock(this->contractsMutex);
  return this->contracts.contains(tx.getTo());
}

bool ContractManager::isContractAddress(const Address &address) const {
  std::shared_lock(this->contractsMutex);
  for (const auto& [protocolName, protocolAddress] : ProtocolContractAddresses) {
    if (address == protocolAddress) return true;
  }
  return this->contracts.contains(address);
}

std::vector<std::pair<std::string, Address>> ContractManager::getContracts() const {
  std::shared_lock lock(this->contractsMutex);
  std::vector<std::pair<std::string, Address>> contracts;
  for (const auto& [address, contract] : this->contracts) {
    contracts.push_back({contract->getContractName(), address});
  }
  return contracts;
}

void ContractManagerInterface::populateBalance(const Address &address) const {
  if (!this->manager.callState->hasBalance(address)) {
    auto it = this->manager.state->accounts.find(address);
    this->manager.callState->setBalanceAt(address,
      (it != this->manager.state->accounts.end()) ? it->second.balance : 0
    );
  }
}

uint256_t ContractManagerInterface::getBalanceFromAddress(const Address& address) const {
  this->populateBalance(address);
  return this->manager.callState->getBalanceAt(address);
}

void ContractManagerInterface::sendTokens(
  const Address& from, const Address& to, const uint256_t& amount
) {
  this->populateBalance(from);
  this->populateBalance(to);
  if (this->manager.callState->getBalanceAt(to) < amount) throw std::runtime_error("Not enough balance");
  this->manager.callState->subBalance(from, amount);
  this->manager.callState->addBalance(to, amount);
}

