#include "contractmanager.h"
#include "contractfactory.h"
#include "contractcalllogger.h"
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
  interface(std::make_unique<ContractManagerInterface>(*this))
{
  this->callLogger = std::make_unique<ContractCallLogger>(*this);
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
  std::shared_lock lock(this->contractsMutex);
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
  this->callLogger = std::make_unique<ContractCallLogger>(*this);
  auto callInfo = tx.txToCallInfo();
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  if (to == this->getContractAddress()) {
    this->callLogger->setContractVars(this, from, from, value);
    try {
      this->ethCall(callInfo);
    } catch (std::exception &e) {
      this->callLogger.reset();
      throw std::runtime_error(e.what());
    }
    this->callLogger->shouldCommit();
    this->callLogger.reset();
    return;
  }

  if (to == ProtocolContractAddresses.at("rdPoS")) {
    this->callLogger->setContractVars(rdpos.get(), from, from, value);
    try {
      rdpos->ethCall(callInfo);
    } catch (std::exception &e) {
      this->callLogger.reset();
      throw std::runtime_error(e.what());
    }
    this->callLogger->shouldCommit();
    this->callLogger.reset();
    return;
  }

  std::unique_lock lock(this->contractsMutex);
  auto it = this->contracts.find(to);
  if (it == this->contracts.end()) {
    this->callLogger.reset();
    throw std::runtime_error(std::string(__func__) + "(void): Contract does not exist");
  }

  const auto& contract = it->second;
  this->callLogger->setContractVars(contract.get(), from, from, value);
  try {
    contract->ethCall(callInfo);
  } catch (std::exception &e) {
    this->callLogger.reset();
    throw std::runtime_error(e.what());
  }

  if (contract->isPayableFunction(functor)) {
    this->state->processContractPayable(this->callLogger->getBalances());
  }
  this->callLogger->shouldCommit();
  this->callLogger.reset();
}

const Bytes ContractManager::callContract(const ethCallInfo& callInfo) const {
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  if (to == this->getContractAddress()) return this->ethCallView(callInfo);
  if (to == ProtocolContractAddresses.at("rdPoS")) return rdpos->ethCallView(callInfo);
  std::shared_lock lock(this->contractsMutex);
  if (!this->contracts.contains(to)) {
    throw std::runtime_error(std::string(__func__) + "(Bytes): Contract does not exist");
  }
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
  this->callLogger = std::make_unique<ContractCallLogger>(*this);
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  try {
    if (value) {
      // Payable, we need to "add" the balance to the contract
      this->interface->populateBalance(from);
      this->interface->populateBalance(to);
      this->callLogger->subBalance(from, value);
      this->callLogger->addBalance(to, value);
    }
    if (to == this->getContractAddress()) {
      this->callLogger->setContractVars(this, from, from, value);
      this->ethCall(callInfo);
      this->callLogger.reset();
      return true;
    }

    if (to == ProtocolContractAddresses.at("rdPoS")) {
      this->callLogger->setContractVars(rdpos.get(), from, from, value);
      rdpos->ethCall(callInfo);
      this->callLogger.reset();
      return true;
    }

    std::shared_lock lock(this->contractsMutex);
    if (!this->contracts.contains(to)) {
      this->callLogger.reset();
      return false;
    }
    const auto &contract = contracts.at(to);
    this->callLogger->setContractVars(contract.get(), from, from, value);
    contract->ethCall(callInfo);
  } catch (std::exception &e) {
    this->callLogger.reset();
    throw std::runtime_error(e.what());
  }
  this->callLogger.reset();
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

void ContractManagerInterface::registerVariableUse(SafeBase& variable) {
  this->manager.callLogger->addUsedVar(variable);
}

void ContractManagerInterface::populateBalance(const Address &address) const {
  if (!this->manager.callLogger) throw std::runtime_error(
    "Contracts going haywire! Trying to call ContractState without an active callContract"
  );
  if (!this->manager.callLogger->hasBalance(address)) {
    auto it = this->manager.state->accounts.find(address);
    this->manager.callLogger->setBalanceAt(address,
      (it != this->manager.state->accounts.end()) ? it->second.balance : 0
    );
  }
}

uint256_t ContractManagerInterface::getBalanceFromAddress(const Address& address) const {
  if (!this->manager.callLogger) throw std::runtime_error(
    "Contracts going haywire! Trying to call ContractState without an active callContract"
  );
  this->populateBalance(address);
  return this->manager.callLogger->getBalanceAt(address);
}

void ContractManagerInterface::sendTokens(
  const Address& from, const Address& to, const uint256_t& amount
) {
  if (!this->manager.callLogger) throw std::runtime_error(
    "Contracts going haywire! Trying to call ContractState without an active callContract"
  );
  this->populateBalance(from);
  this->populateBalance(to);
  if (this->manager.callLogger->getBalanceAt(from) < amount) {
    throw std::runtime_error("ContractManager::sendTokens: Not enough balance");
  }
  this->manager.callLogger->subBalance(from, amount);
  this->manager.callLogger->addBalance(to, amount);
}

