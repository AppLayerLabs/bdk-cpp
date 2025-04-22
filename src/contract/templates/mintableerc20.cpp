/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "mintableerc20.h"

ERC20Mintable::ERC20Mintable(const Address& address, const DB& db
) : DynamicContract(address, db), ERC20(address, db), Ownable(address, db) {
  this->registerContractFunctions();
}

ERC20Mintable::ERC20Mintable(
  const std::string &erc20_name, const std::string &erc20_symbol,
  const uint8_t &erc20_decimals,
  const Address &address, const Address &creator,
  const uint64_t &chainId
) :  DynamicContract("ERC20Mintable", address, creator, chainId),
    ERC20("ERC20Mintable", erc20_name, erc20_symbol, erc20_decimals,
  0, address, creator, chainId), Ownable("ERC20Mintable", creator, address, creator, chainId) {
  this->registerContractFunctions();
}

ERC20Mintable::~ERC20Mintable() = default;

DBBatch ERC20Mintable::dump() const {
  // We need to dump all the data from the parent class as well
  DBBatch batch = ERC20::dump();
  auto ownableBatch = Ownable::dump();
  for (const auto& dbItem : ownableBatch.getPuts()) batch.push_back(dbItem);
  for (const auto& dbItem : ownableBatch.getDels()) batch.delete_key(dbItem);
  return batch;
}

void ERC20Mintable::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("mint", &ERC20Mintable::mint, FunctionTypes::NonPayable, this);
}

void ERC20Mintable::mint(const Address &to, const uint256_t &amount) {
  this->onlyOwner();
  ERC20::mint_(to, amount);
}

