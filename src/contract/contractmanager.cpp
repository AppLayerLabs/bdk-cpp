/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contractmanager.h"
#include "abi.h"
#include "contractfactory.h"
#include "customcontracts.h"
#include "../core/rdpos.h"
#include "../core/state.h"
#include "../utils/dynamicexception.h"
#include "contracthost.h"

ContractManager::ContractManager(DB& db,
                                 std::unordered_map<Address, std::unique_ptr<BaseContract>, SafeHash>& contracts,
                                 const Options& options)
: BaseContract("ContractManager", ProtocolContractAddresses.at("ContractManager"), options.getChainOwner(), options.getChainID(), db),
  contracts_(contracts)
{
  ContractFactory::registerContracts<ContractTypes>();
  ContractFactory::addAllContractFuncs<ContractTypes>(this->createContractFuncs_);
  // Load Contracts from DB
  std::vector<DBEntry> contractsFromDB = this->db_.getBatch(DBPrefix::contractManager);
  for (const DBEntry& contract : contractsFromDB) {
    Address address(contract.key);
    if (!this->loadFromDB<ContractTypes>(contract, address)) {
      throw DynamicException("Unknown contract: " + Utils::bytesToString(contract.value));
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
  this->db_.putBatch(contractsBatch);
}

Bytes ContractManager::getDeployedContracts() const {
  std::vector<std::string> names;
  std::vector<Address> addresses;
  for (const auto& [address, contract] : this->contracts_) {
    names.push_back(contract->getContractName());
    addresses.push_back(address);
  }
  Bytes result = ABI::Encoder::encodeData(names, addresses);
  return result;
}

void ContractManager::ethCall(const ethCallInfo& callInfo, ContractHost* host) {
  PointerNullifier nullifier(this->host_);
  const auto& caller = std::get<0>(callInfo);
  const Functor& functor = std::get<5>(callInfo);
  /// Call the function on this->createContractFuncs_
  auto it = this->createContractFuncs_.find(functor);
  if (it == this->createContractFuncs_.end()) {
    throw DynamicException("ContractManager: Invalid function call");
  }
  it->second(callInfo,
             ContractHost::deriveContractAddress(this->host_->getNonce(caller), caller),
             this->contracts_,
             this->getContractChainId(),
             this->db_);
}

Bytes ContractManager::ethCallView(const ethCallInfo& callInfo, ContractHost* host) const {
  PointerNullifier nullifier(this->host_);
  // This hash is equivalent to "function getDeployedContracts() public view returns (string[] memory, address[] memory) {}"
  if (std::get<5>(callInfo) == Hex::toBytes("0xaa9a068f")) return this->getDeployedContracts();
  throw DynamicException("Invalid function call");
}