/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contractmanager.h"
#include "contractfactory.h"
#include "customcontracts.h"

ContractManager::ContractManager(
  ContractsContainerType& contracts,
  CreateContractFuncsType& createContractFuncs,
  const Options& options
) : BaseContract(
      "ContractManager",
      ProtocolContractAddresses.at("ContractManager"),
      options.getChainOwner(), // contract manager creator is the chain owner address
      options.getChainID()
    ),
    contracts_(contracts),
    createContractFuncs_(createContractFuncs)
{
  // NOTE: Keep this ctor body empty.
  //       ContractManager is now a stateless class; it just keeps references
  //       to data kept at a State instance.
}

ContractManager::~ContractManager() {}

DBBatch ContractManager::dump() const {
  DBBatch contractsBatch;
  for (const auto& [address, contract] : this->contracts_) {
    if (typeid(*contract) == typeid(ContractManager)) continue;
    contractsBatch.push_back(
      Bytes(address.asBytes()),
      StrConv::stringToBytes(contract->getContractName()),
      DBPrefix::contractManager
    );
  }
  return contractsBatch;
}

std::vector<std::tuple<std::string, Address>> ContractManager::getDeployedContracts() const {
  std::vector<std::tuple<std::string, Address>> contracts;
  for (const auto& [address, contract] : this->contracts_) {
    std::tuple contractTuple(contract->getContractName(), address);
    contracts.push_back(contractTuple);
  }
  return contracts;
}

std::vector<std::tuple<std::string, Address>> ContractManager::getDeployedContractsForCreator(const Address& creator) const {
  std::vector<std::tuple<std::string, Address>> contracts;
  for (const auto& [address, contract] : this->contracts_) {
    if (contract->getContractCreator() == creator) {
      std::tuple contractTuple(contract->getContractName(), address);
      contracts.push_back(contractTuple);
    }
  }
  return contracts;
}

void ContractManager::ethCall(const evmc_message& callInfo, ContractHost* host) {
  this->host_ = host;
  PointerNullifier nullifier(this->host_);
  const Address caller(callInfo.sender);
  const Functor functor = EVMCConv::getFunctor(callInfo);
  // Call the function on this->createContractFuncs_
  auto it = this->createContractFuncs_.find(functor);
  if (it == this->createContractFuncs_.end()) {
    throw DynamicException("ContractManager: Invalid function call");
  }
  it->second(callInfo,
             generateContractAddress(this->host_->context().getAccount(caller).getNonce(), caller),
             this->contracts_,
             this->getContractChainId(),
             this->host_);
}

Bytes ContractManager::evmEthCall(const evmc_message& callInfo, ContractHost* host) {
  this->ethCall(callInfo, host);
  return Bytes();
}

Bytes ContractManager::ethCallView(const evmc_message& callInfo, ContractHost* host) const {
  PointerNullifier nullifier(this->host_);
  // This hash is equivalent to "function getDeployedContracts() public view returns (Contract[] memory) {}"
  // 0xaa9a068f == uint32_t(2862220943);
  auto functor = EVMCConv::getFunctor(callInfo);
  if (functor.value == 2862220943) return ABI::Encoder::encodeData(this->getDeployedContracts());
  // This hash is equivalent to "function getDeployedContractsForCreator(address creator) public view returns (Contract[] memory) {}"
  // 0x73474f5a == uint32_t(1934053210)
  if (functor.value == 1934053210) {
    auto args = EVMCConv::getFunctionArgs(callInfo);
    auto [addr] = ABI::Decoder::decodeData<Address>(args);
    return ABI::Encoder::encodeData(this->getDeployedContractsForCreator(addr));
  }
  throw DynamicException("Invalid function call");
}

