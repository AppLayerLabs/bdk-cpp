#include "contractmanager.h"
#include "erc20.h"
#include "../core/rdpos.h"

ContractManager::ContractManager(const std::unique_ptr<DB>& db, const std::unique_ptr<rdPoS>& rdpos, const std::unique_ptr<Options>& options) :
  BaseContract("ContractManager", ProtocolContractAddresses.at("ContractManager"), Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true), 0, db), rdpos(rdpos), options(options) {
  /// Load Contracts from DB.
  auto contracts = this->db->getBatch(DBPrefix::contractManager);
  for (const auto& contract : contracts) {
   if (contract.value == "ERC20") {
     Address contractAddress(contract.key, true);
     this->contracts.insert(std::make_pair(contractAddress, std::make_unique<ERC20>(contractAddress, this->db)));
     continue;
   }
    throw std::runtime_error("Unknown contract: " + contract.value);
  }
}

ContractManager::~ContractManager() {
  DBBatch contractsBatch;
  for (const auto& [contractAddress, contract] : this->contracts) {
    contractsBatch.puts.push_back(DBEntry(contractAddress.get(), contract->getContractName()));
  }
  this->db->putBatch(contractsBatch, DBPrefix::contractManager);
}

Address ContractManager::deriveContractAddress(const TxBlock& tx) const {
  /// Contract address = sha3(rlp(tx.from() + tx.nonce()).substr(12);
  uint8_t rlpSize = 0xc0;
  rlpSize += this->getCaller().size();
  rlpSize += (tx.getNonce() < 0x80) ? 1 : 1 + Utils::bytesRequired(tx.getNonce());
  std::string rlp;
  rlp += rlpSize;
  rlp += this->getCaller().get();
  rlp += (tx.getNonce() < 0x80) ? (char)tx.getNonce() : (char)0x80 + Utils::bytesRequired(tx.getNonce());
  return Address(Utils::sha3(rlp).get().substr(12), true);
}

void ContractManager::createNewERC20Contract(const TxBlock& tx) {
  if (this->caller != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can create new contracts");
  }
  /// Check if desired contract address already exists
  const auto derivedContractAddress = this->deriveContractAddress(tx);
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
  ABI::Decoder decoder(types, tx.getData().substr(4));

  /// Check if decimals are within range
  if (decoder.getData<uint256_t>(2) > 255) {
    throw std::runtime_error("Decimals must be between 0 and 255");
  }

  /// Create the contract
  this->contracts.insert(std::make_pair(derivedContractAddress, std::make_unique<ERC20>(decoder.getData<std::string>(0),
                                                                                        decoder.getData<std::string>(1),
                                                                                        uint8_t(decoder.getData<uint256_t>(2)),
                                                                                        decoder.getData<uint256_t>(3),
                                                                                        derivedContractAddress,
                                                                                        this->getCaller(),
                                                                                        this->options->getChainID(),
                                                                                        this->db)));
  return;
}

void ContractManager::validateCreateNewERC20Contract(const TxBlock &tx) {
  if (this->caller != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can create new contracts");
  }
  /// Check if desired contract address already exists
  const auto derivedContractAddress = this->deriveContractAddress(tx);
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
  ABI::Decoder decoder(types, tx.getData().substr(4));

  /// Check if decimals are within range
  if (decoder.getData<uint256_t>(2) > 255) {
    throw std::runtime_error("Decimals must be between 0 and 255");
  }

  return;
}

void ContractManager::ethCall(const TxBlock& tx) {
  this->origin = tx.getFrom();
  this->caller = tx.getFrom();
  std::string functor = tx.getData().substr(0, 4);

  /// function createNewERC20Contract(string memory name, string memory symbol, uint8 decimals, uint256 supply) public {}
  if (functor == Hex::toBytes("0xb74e5ed5")) {
    this->createNewERC20Contract(tx);
    return;
  }
  throw std::runtime_error("Invalid function call");
}

void ContractManager::ethCall(const ethCallInfo& callInfo) {
  auto [from, to, gasLimit, gasPrice, value, data] = callInfo;
  this->origin = from;
  this->caller = from;
  PrivKey mockupPrivKey(Hex::toBytes("2a616d189193e56994f22993ac4eb4dca0e2652afdc95d240739837ab83b21e2"));
  Address mockupAddr = Secp256k1::toAddress(Secp256k1::toUPub(mockupPrivKey));
  TxBlock tx(
    mockupAddr,
    this->getContractAddress(),
    data,
    this->getContractChainId(),
    0,
    value,
    gasPrice,
    gasPrice,
    gasLimit,
    mockupPrivKey
  );

  std::string functor = data.substr(0, 4);
  /// function createNewERC20Contract(string memory name, string memory symbol, uint8 decimals, uint256 supply) public {}
  if (functor == Hex::toBytes("0xb74e5ed5")) {
    this->validateCreateNewERC20Contract(tx);
    return;
  }

  throw std::runtime_error("Invalid function call");
}

std::string ContractManager::getDeployedContracts() const {
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
  return ABI::Encoder(vars).getRaw();
}

const std::string ContractManager::ethCall(const std::string& data) const {
  std::string functor = data.substr(0, 4);

  /// function getDeployedContracts() public view returns (string[] memory, address[] memory) {}
  if (functor == Hex::toBytes("0xaa9a068f")) {
    return this->getDeployedContracts();
  }

  throw std::runtime_error("Invalid function call");
}

void ContractManager::callContract(const TxBlock& tx) {
  if (tx.getTo() == this->getContractAddress()) {
    this->caller = tx.getFrom();
    this->origin = tx.getFrom();
    this->value = tx.getValue();
    this->ethCall(tx);
    return;
  }

  if (tx.getTo() == ProtocolContractAddresses.at("rdPoS")) {
    rdpos->caller = tx.getFrom();
    rdpos->origin = tx.getFrom();
    rdpos->value = tx.getValue();
    rdpos->ethCall(tx);
    return;
  }

  std::unique_lock lock(this->contractsMutex);
  if (!this->contracts.contains(tx.getTo())) {
    throw std::runtime_error("Contract does not exist");
  }

  if (tx.getValue()) {
    const auto& contract = contracts.at(tx.getTo());
    contract->caller = tx.getFrom();
    contract->origin = tx.getFrom();
    contract->value = tx.getValue();
    contract->ethCall(tx, tx.getValue());
  } else {
    const auto& contract = contracts.at(tx.getTo());
    contract->caller = tx.getFrom();
    contract->origin = tx.getFrom();
    contract->value = 0;
    contract->ethCall(tx);
  }
}

bool ContractManager::validateCallContractWithTx(const ethCallInfo& callInfo) {
  const auto& [from, to, gasLimit, gasPrice, value, data] = callInfo;

  if (to == this->getContractAddress()) {
    this->caller = from;
    this->origin = from;
    this->value = value;
    this->ethCall(callInfo);
    return true;
  }

  if (to == ProtocolContractAddresses.at("rdPoS")) {
    rdpos->caller = from;
    rdpos->origin = from;
    rdpos->value = value;
    rdpos->ethCall(callInfo);
    return true;
  }

  std::shared_lock lock(this->contractsMutex);
  if (!this->contracts.contains(to)) {
    return false;
  }
  const auto& contract = contracts.at(to);
  contract->caller = from;
  contract->origin = from;
  contract->value = value;
  contract->ethCall(callInfo);
  return true;
}

std::string ContractManager::callContract(const Address& address, const std::string& data) const {
  if (address == this->getContractAddress()) {
    return this->ethCall(data);
  }

  if (address == ProtocolContractAddresses.at("rdPoS")) {
    return rdpos->ethCall(data);
  }

  std::shared_lock lock(this->contractsMutex);
  if (!this->contracts.contains(address)) {
    throw std::runtime_error("Contract does not exist");
  }
  return this->contracts.at(address)->ethCall(data);
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
