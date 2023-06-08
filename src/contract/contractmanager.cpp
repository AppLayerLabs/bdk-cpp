#include "contractmanager.h"
#include "erc20.h"
#include "erc20wrapper.h"
#include "nativewrapper.h"
#include "../core/rdpos.h"
#include "../core/state.h"

ContractManager::ContractManager(State* state, const std::unique_ptr<DB>& db, const std::unique_ptr<rdPoS>& rdpos, const std::unique_ptr<Options>& options) :
  state(state), BaseContract("ContractManager", ProtocolContractAddresses.at("ContractManager"), Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), 0, db),
  rdpos(rdpos),
  options(options),
  interface(*this) {
  /// Load Contracts from DB.
  auto contracts = this->db->getBatch(DBPrefix::contractManager);
  for (const auto& contract : contracts) {
   if (Utils::bytesToString(contract.value) == "ERC20") {
     Address contractAddress(contract.key);
     this->contracts.insert(std::make_pair(contractAddress, std::make_unique<ERC20>(this->interface, contractAddress, this->db)));
     continue;
   }
  if (Utils::bytesToString(contract.value) == "ERC20Wrapper") {
    Address contractAddress(contract.key);
    this->contracts.insert(
    std::make_pair(contractAddress, std::make_unique<ERC20Wrapper>(this->interface, contractAddress, this->db)));
    continue;
  }
  if (Utils::bytesToString(contract.value) == "NativeWrapper") {
    Address contractAddress(contract.key);
    this->contracts.insert(
    std::make_pair(contractAddress, std::make_unique<NativeWrapper>(this->interface, contractAddress, this->db)));
    continue;
  }

    throw std::runtime_error("Unknown contract: " + Utils::bytesToString(contract.value));
  }
}

ContractManager::~ContractManager() {
  DBBatch contractsBatch;
  for (const auto& [contractAddress, contract] : this->contracts) {
    contractsBatch.push_back(
        Bytes(contractAddress.asBytes()),
        Utils::stringToBytes(contract->getContractName()),
        DBPrefix::contractManager
      );
  }
  this->db->putBatch(contractsBatch);
}

Address ContractManager::deriveContractAddress(const ethCallInfo& callInfo) const {
  /// Contract address = sha3(rlp(tx.from() + tx.nonce()).substr(12);
  uint8_t rlpSize = 0xc0;
  rlpSize += this->getCaller().size();
  /// As we don't have actually access to the nonce, we will use the number of contracts existing in the chain
  rlpSize += (this->contracts.size() < 0x80) ? 1 : 1 + Utils::bytesRequired(this->contracts.size());
  Bytes rlp;
  rlp.insert(rlp.end(), rlpSize);
  rlp.insert(rlp.end(), this->getCaller().cbegin(), this->getCaller().cend());
  rlp.insert(rlp.end(), (this->contracts.size() < 0x80) ? (char)this->contracts.size() : (char)0x80 + Utils::bytesRequired(this->contracts.size()));
  return Address(Utils::sha3(rlp).view_const(12));
}

void ContractManager::createNewERC20Contract(const ethCallInfo& callInfo) {
  if (this->caller != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can create new contracts");
  }
  /// Check if desired contract address already exists
  const auto derivedContractAddress = this->deriveContractAddress(callInfo);
  if (this->contracts.contains(derivedContractAddress)) {
    throw std::runtime_error("Contract already exists");
  }

  std::unique_lock lock(this->contractsMutex);
  for (const auto& [protocolContractName, protocolContractAddress] : ProtocolContractAddresses) {
    if (protocolContractAddress == derivedContractAddress) {
      throw std::runtime_error("Contract already exists");
    }
  }

  /// Parse the constructor ABI
  std::vector<ABI::Types> types = { ABI::Types::string, ABI::Types::string, ABI::Types::uint256, ABI::Types::uint256};
  ABI::Decoder decoder(types, std::get<6>(callInfo));

  /// Check if decimals are within range
  if (decoder.getData<uint256_t>(2) > 255) {
    throw std::runtime_error("Decimals must be between 0 and 255");
  }

  /// Create the contract
  this->contracts.insert(std::make_pair(derivedContractAddress, std::make_unique<ERC20>(this->interface,
                                                                                        decoder.getData<std::string>(0),
                                                                                        decoder.getData<std::string>(1),
                                                                                        uint8_t(decoder.getData<uint256_t>(2)),
                                                                                        decoder.getData<uint256_t>(3),
                                                                                        derivedContractAddress,
                                                                                        this->getCaller(),
                                                                                        this->options->getChainID(),
                                                                                        this->db)));
  return;
}

void ContractManager::validateCreateNewERC20Contract(const ethCallInfo &callInfo) const {
  if (this->caller != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can create new contracts");
  }
  /// Check if desired contract address already exists
  const auto derivedContractAddress = this->deriveContractAddress(callInfo);
  {
    std::shared_lock lock(this->contractsMutex);
    if (this->contracts.contains(derivedContractAddress)) {
      throw std::runtime_error("Contract already exists");
    }
  }

  for (const auto& [protocolContractName, protocolContractAddress] : ProtocolContractAddresses) {
    if (protocolContractAddress == derivedContractAddress) {
      throw std::runtime_error("Contract already exists");
    }
  }

  /// Parse the constructor ABI
  std::vector<ABI::Types> types = { ABI::Types::string, ABI::Types::string, ABI::Types::uint256, ABI::Types::uint256};
  ABI::Decoder decoder(types, std::get<6>(callInfo));

  /// Check if decimals are within range
  if (decoder.getData<uint256_t>(2) > 255) {
    throw std::runtime_error("Decimals must be between 0 and 255");
  }

  return;
}

void ContractManager::createNewERC20WrapperContract(const ethCallInfo& callInfo) {
  if (this->caller != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can create new contracts");
  }
  /// Check if desired contract address already exists
  const auto derivedContractAddress = this->deriveContractAddress(callInfo);
  if (this->contracts.contains(derivedContractAddress)) {
    throw std::runtime_error("Contract already exists");
  }

  std::unique_lock lock(this->contractsMutex);
  for (const auto& [protocolContractName, protocolContractAddress] : ProtocolContractAddresses) {
    if (protocolContractAddress == derivedContractAddress) {
      throw std::runtime_error("Contract already exists");
    }
  }

  this->contracts.insert(std::make_pair(derivedContractAddress, std::make_unique<ERC20Wrapper>(this->interface,
                                                                                               derivedContractAddress,
                                                                                               this->getCaller(),
                                                                                               this->options->getChainID(),
                                                                                               this->db)));
}

void ContractManager::validateCreateNewERC20WrapperContract(const ethCallInfo& callInfo) const {
  if (this->caller != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can create new contracts");
  }
  /// Check if desired contract address already exists
  const auto derivedContractAddress = this->deriveContractAddress(callInfo);
  if (this->contracts.contains(derivedContractAddress)) {
    throw std::runtime_error("Contract already exists");
  }

  std::unique_lock lock(this->contractsMutex);
  for (const auto& [protocolContractName, protocolContractAddress] : ProtocolContractAddresses) {
    if (protocolContractAddress == derivedContractAddress) {
      throw std::runtime_error("Contract already exists");
    }
  }
}

void ContractManager::createNewERC20NativeWrapperContract(const ethCallInfo& callInfo) {
  if (this->caller != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can create new contracts");
  }
  /// Check if desired contract address already exists
  const auto derivedContractAddress = this->deriveContractAddress(callInfo);
  if (this->contracts.contains(derivedContractAddress)) {
    throw std::runtime_error("Contract already exists");
  }

  std::unique_lock lock(this->contractsMutex);
  for (const auto& [protocolContractName, protocolContractAddress] : ProtocolContractAddresses) {
    if (protocolContractAddress == derivedContractAddress) {
      throw std::runtime_error("Contract already exists");
    }
  }

  /// Parse the constructor ABI
  std::vector<ABI::Types> types = { ABI::Types::string, ABI::Types::string, ABI::Types::uint256 };
  ABI::Decoder decoder(types, std::get<6>(callInfo));

  /// Check if decimals are within range
  if (decoder.getData<uint256_t>(2) > 255) {
    throw std::runtime_error("Decimals must be between 0 and 255");
  }

  /// Create the contract
  this->contracts.insert(std::make_pair(derivedContractAddress, std::make_unique<NativeWrapper>(this->interface,
                                                                                        decoder.getData<std::string>(0),
                                                                                        decoder.getData<std::string>(1),
                                                                                        uint8_t(decoder.getData<uint256_t>(2)),
                                                                                        derivedContractAddress,
                                                                                        this->getCaller(),
                                                                                        this->options->getChainID(),
                                                                                        this->db)));
}

void ContractManager::validateCreateNewERC20NativeWrapperContract(const ethCallInfo &callInfo) const {
  if (this->caller != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can create new contracts");
  }
  /// Check if desired contract address already exists
  const auto derivedContractAddress = this->deriveContractAddress(callInfo);
  if (this->contracts.contains(derivedContractAddress)) {
    throw std::runtime_error("Contract already exists");
  }

  std::unique_lock lock(this->contractsMutex);
  for (const auto& [protocolContractName, protocolContractAddress] : ProtocolContractAddresses) {
    if (protocolContractAddress == derivedContractAddress) {
      throw std::runtime_error("Contract already exists");
    }
  }

  /// Parse the constructor ABI
  std::vector<ABI::Types> types = { ABI::Types::string, ABI::Types::string, ABI::Types::uint256 };
  ABI::Decoder decoder(types, std::get<6>(callInfo));

  /// Check if decimals are within range
  if (decoder.getData<uint256_t>(2) > 255) {
    throw std::runtime_error("Decimals must be between 0 and 255");
  }
}

void ContractManager::ethCall(const ethCallInfo& callInfo) {
  Functor functor = std::get<5>(callInfo);
  if (this->getCommit()) {
    /// function createNewERC20Contract(string memory name, string memory symbol, uint8 decimals, uint256 supply) public {}
    if (functor == Hex::toBytes("0xb74e5ed5")) {
      this->createNewERC20Contract(callInfo);
      return;
    }
    /// function createNewERC20WrapperContract() public {}
    if (functor == Hex::toBytes("0x97aa51a3")) {
      this->createNewERC20WrapperContract(callInfo);
      return;
    }
    /// function createNewERC20NativeWrapperContract(string memory name, string memory symbol, uint8 decimals) public {}
    if (functor == Hex::toBytes("0x9f90f4c7")) {
      this->createNewERC20NativeWrapperContract(callInfo);
      return;
    }
  } else {
    if (functor == Hex::toBytes("0xb74e5ed5")) {
      this->validateCreateNewERC20Contract(callInfo);
      return;
    }
    if (functor == Hex::toBytes("0x97aa51a3")) {
      this->validateCreateNewERC20WrapperContract(callInfo);
      return;
    }
    if (functor == Hex::toBytes("0x9f90f4c7")) {
      this->validateCreateNewERC20NativeWrapperContract(callInfo);
      return;
    }
  }
  throw std::runtime_error("Invalid function call");
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

const Bytes ContractManager::ethCallView(const ethCallInfo& data) const {
  const auto& functor = std::get<5>(data);

  /// function getDeployedContracts() public view returns (string[] memory, address[] memory) {}
  if (functor == Hex::toBytes("0xaa9a068f")) {
    return this->getDeployedContracts();
  }

  throw std::runtime_error("Invalid function call");
}

void ContractManager::callContract(const TxBlock& tx) {
  this->commit = true;
  auto callInfo = tx.txToCallInfo();
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  if (to == this->getContractAddress()) {
    this->caller = from;
    this->origin = from;
    this->value = value;
    try {
      this->ethCall(callInfo);
    } catch (std::exception &e) {
      this->commit = false;
      balances.clear();
      throw std::runtime_error(e.what());
    }
    this->commit = false;
    balances.clear();
    return;
  }

  if (to == ProtocolContractAddresses.at("rdPoS")) {
    rdpos->caller = from;
    rdpos->origin = from;
    rdpos->value = value;
    rdpos->commit = this->commit;
    try {
      rdpos->ethCall(callInfo);
    } catch (std::exception &e) {
      rdpos->commit = false;
      balances.clear();
      throw std::runtime_error(e.what());
    }
    rdpos->commit = false;
    balances.clear();
    return;
  }

  std::unique_lock lock(this->contractsMutex);
  if (!this->contracts.contains(to)) {
    balances.clear();
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
    balances.clear();
    throw std::runtime_error(e.what());
  }

  if (contract->isPayableFunction(functor)) {
    this->state->processContractPayable(this->balances);
  }

  balances.clear();
  contract->commit = false;
}

bool ContractManager::isPayable(const ethCallInfo& callInfo) const {
  const auto& address = std::get<1>(callInfo);
  const auto& functor = std::get<5>(callInfo);

  std::shared_lock lock(this->contractsMutex);

  auto it = this->contracts.find(address);
  if (it == this->contracts.end()) {
    return false;
  }

  return it->second->isPayableFunction(functor);
}

bool ContractManager::validateCallContractWithTx(const ethCallInfo& callInfo) {
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  try {
    if (this->getValue()) {
      /// Payable, we need to "add" the balance to the contract
      this->interface.populateBalance(to);
      this->balances[to] += value;
    }
    if (to == this->getContractAddress()) {
      this->caller = from;
      this->origin = from;
      this->value = value;
      this->ethCall(callInfo);
      balances.clear();
      return true;
    }

    if (to == ProtocolContractAddresses.at("rdPoS")) {
      rdpos->caller = from;
      rdpos->origin = from;
      rdpos->value = value;
      rdpos->commit = false;
      rdpos->ethCall(callInfo);
      balances.clear();
      return true;
    }

    std::shared_lock lock(this->contractsMutex);
    if (!this->contracts.contains(to)) {
      balances.clear();
      return false;
    }
    const auto &contract = contracts.at(to);
    contract->caller = from;
    contract->origin = from;
    contract->value = value;
    contract->commit = false;
    contract->ethCall(callInfo);
  } catch (std::exception &e) {
    balances.clear();
    throw std::runtime_error(e.what());
  }
  balances.clear();
  return true;
}

const Bytes ContractManager::callContract(const ethCallInfo& callInfo) const {
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  if (to == this->getContractAddress()) {
    return this->ethCallView(callInfo);
  }

  if (to == ProtocolContractAddresses.at("rdPoS")) {
    return rdpos->ethCallView(callInfo);
  }

  std::shared_lock lock(this->contractsMutex);
  if (!this->contracts.contains(to)) {
    throw std::runtime_error("Contract does not exist");
  }
  return this->contracts.at(to)->ethCallView(callInfo);
}

bool ContractManager::isContractCall(const TxBlock &tx) const {
  if (tx.getTo() == this->getContractAddress()) {
    return true;
  }
  for (const auto& [protocolContractName, protocolContractAddress] : ProtocolContractAddresses) {
    if (tx.getTo() == protocolContractAddress) {
      return true;
    }
  }

  std::shared_lock lock(this->contractsMutex);
  return this->contracts.contains(tx.getTo());
}

bool ContractManager::isContractAddress(const Address &address) const {
  std::shared_lock(this->contractsMutex);
  for (const auto& [protocolContractName, protocolContractAddress] : ProtocolContractAddresses) {
    if (address == protocolContractAddress) {
      return true;
    }
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

void ContractManager::ContractManagerInterface::callContract(const ethCallInfo& callInfo) {
  const auto& [from, to, gasLimit, gasPrice, value, functor, data] = callInfo;
  if (value) {
    this->sendTokens(from, to, value);
  }

  if (!this->contractManager.contracts.contains(to)) {
    throw std::runtime_error("Contract does not exist");
  }

  const auto& contract = this->contractManager.contracts.at(to);
  contract->caller = from;
  contract->value = value;
  contract->commit = this->contractManager.getCommit();
  try {
    contract->ethCall(callInfo);
  } catch (std::exception &e) {
    contract->commit = false;
    throw std::runtime_error(e.what());
  }
}

void ContractManager::ContractManagerInterface::populateBalance(const Address &address) const {
  if (!this->contractManager.balances.contains(address)) {
    auto it = this->contractManager.state->accounts.find(address);
    if (it != this->contractManager.state->accounts.end()) {
      this->contractManager.balances[address] = it->second.balance;
    } else {
      this->contractManager.balances[address] = 0;
    }
  }
}

uint256_t ContractManager::ContractManagerInterface::getBalanceFromAddress(const Address& address) const {
  this->populateBalance(address);
  return this->contractManager.balances[address];
}

void ContractManager::ContractManagerInterface::sendTokens(const Address& from, const Address& to, const uint256_t& amount) {
  this->populateBalance(from);
  this->populateBalance(to);

  if (this->contractManager.balances[to] < amount) {
    throw std::runtime_error("Not enough balance");
  }

  this->contractManager.balances[from] -= amount;
  this->contractManager.balances[to] += amount;
}