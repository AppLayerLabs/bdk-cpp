#include "contractmanager.h"
#include "erc20.h"
#include "../core/rdpos.h"

ContractManager::ContractManager(const std::unique_ptr<DB>& db, std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Options>& options) :
  Contract("ContractManager", ProtocolContractAddresses.at("ContractManager"), Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true), 0, db), rdpos(rdpos), options(options) {
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
  rlpSize += tx.getFrom().size();
  rlpSize += (tx.getNonce() < 0x80) ? 1 : 1 + Utils::bytesRequired(tx.getNonce());
  std::string rlp;
  rlp += rlpSize;
  rlp += tx.getFrom().get();
  rlp += (tx.getNonce() < 0x80) ? (char)tx.getNonce() : (char)0x80 + Utils::bytesRequired(tx.getNonce());
  return Address(Utils::sha3(rlp).get().substr(12), true);
}

void ContractManager::createNewERC20Contract(const TxBlock& tx) {
  if (tx.getFrom() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can create new contracts");
  }
  /// Check if desired contract address already exists
  const auto derivedContractAddress = this->deriveContractAddress(tx);
  if (this->contracts.find(derivedContractAddress) != this->contracts.end()) {
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
                                                                                        tx.getFrom(),
                                                                                        this->options->getChainID(),
                                                                                        this->db)));
  return;
}

void ContractManager::validateCreateNewERC20Contract(const TxBlock &tx) {
  if (tx.getFrom() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can create new contracts");
  }
  /// Check if desired contract address already exists
  const auto derivedContractAddress = this->deriveContractAddress(tx);
  {
    std::shared_lock lock(this->contractsMutex);
    if (this->contracts.find(derivedContractAddress) != this->contracts.end()) {
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

void ContractManager::ethCall(const TxBlock& tx, bool commit) {
  std::string functor = tx.getData().substr(0, 4);

  /// function createNewERC20Contract(string memory name, string memory symbol, uint8 decimals, uint256 supply) public {}
  if (functor == Hex::toBytes("0xb74e5ed5")) {
    if (commit) {
      this->createNewERC20Contract(tx);
      return;
    } else {
      this->validateCreateNewERC20Contract(tx);
      return;
    }
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
  try {
    if (tx.getTo() == this->getContractAddress()) {
      this->ethCall(tx, true);
      return;
    }

    if (tx.getTo() == ProtocolContractAddresses.at("rdPoS")) {
      rdpos->ethCall(tx, true);
      return;
    }

    std::unique_lock lock(this->contractsMutex);
    if (this->contracts.find(tx.getTo()) == this->contracts.end()) {
      throw std::runtime_error("Contract does not exist");
    }
    this->contracts.at(tx.getTo())->ethCall(tx, true);
  } catch (std::exception &e) {
    Utils::logToDebug(Log::contractManager, __func__, "Transaction Reverted: " + tx.hash().hex().get() + " " + e.what());
  }
}

bool ContractManager::validateCallContract(const TxBlock& tx) {
  try {
    if (tx.getTo() == this->getContractAddress()) {
      this->ethCall(tx, false);
      return true;
    }
    if (tx.getTo() == ProtocolContractAddresses.at("rdPoS")) {
      rdpos->ethCall(tx, false);
      return true;
    }

    std::shared_lock lock(this->contractsMutex);
    if (this->contracts.find(tx.getTo()) == this->contracts.end()) {
      return false;
    }
    this->contracts.at(tx.getTo())->ethCall(tx, false);
    return true;
  } catch (std::exception& e) {
    Utils::logToDebug(Log::contractManager, __func__, "Invalid contract call by tx: " + tx.hash().hex().get() + " " + e.what());
    return false;
  }
}

std::string ContractManager::callContract(const Address& address, const std::string& data) const {
  if (address == this->getContractAddress()) {
    return this->ethCall(data);
  }

  if (address == ProtocolContractAddresses.at("rdPoS")) {
    return rdpos->ethCall(data);
  }

  std::shared_lock lock(this->contractsMutex);
  if (this->contracts.find(address) == this->contracts.end()) {
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
  if (this->contracts.find(tx.getTo()) != this->contracts.end()) {
    return true;
  }
  return false;
}

std::vector<std::pair<std::string, Address>> ContractManager::getContracts() const {
  std::shared_lock lock(this->contractsMutex);
  std::vector<std::pair<std::string, Address>> contracts;
  for (const auto& [address, contract] : this->contracts) {
    contracts.push_back({contract->getContractName(), address});
  }
  return contracts;
}
