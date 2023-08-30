/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "erc721.h"


ERC721::ERC721(ContractManagerInterface& interface, const Address& address, const std::unique_ptr<DB>& db)
  : DynamicContract(interface, address, db), name_(this), symbol_(this), owners_(this), balances_(this), tokenApprovals_(this), operatorAddressApprovals_(this) {

  this->name_ = Utils::bytesToString(db->get(std::string("name_"), this->getDBPrefix()));
  this->symbol_ = Utils::bytesToString(db->get(std::string("symbol_"), this->getDBPrefix()));

  auto owners = db->getBatch(this->getNewPrefix("owners_"));
  for (const auto& dbEntry : owners) {
    BytesArrView valueView(dbEntry.value);
    this->owners_[Utils::fromBigEndian<uint256_t>(dbEntry.key)] = Address(valueView.subspan(0, 20));
  }

  auto balances = db->getBatch(this->getNewPrefix("balances_"));
  for (const auto& dbEntry : balances) {
    this->balances_[Address(dbEntry.key)] = Utils::fromBigEndian<uint256_t>(dbEntry.value);
  }

  auto approvals = db->getBatch(this->getNewPrefix("tokenApprovals_"));
  for (const auto& dbEntry : approvals) {
    this->tokenApprovals_[Utils::fromBigEndian<uint256_t>(dbEntry.key)] = Address(dbEntry.value);
  }

  auto operatorAddressApprovals = db->getBatch(this->getNewPrefix("operatorAddressApprovals_"));
  for (const auto& dbEntry : operatorAddressApprovals) {
    BytesArrView valueView(dbEntry.value);
    this->operatorAddressApprovals_[Address(dbEntry.key)][Address(valueView.subspan(0, 20))] = valueView[20];
  }

  this->registerContractFunctions();
}

ERC721::ERC721(
  const std::string &erc721name, const std::string &erc721symbol_,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "ERC721", address, creator, chainId, db), name_(this, erc721name),
  symbol_(this, erc721symbol_), owners_(this), balances_(this), tokenApprovals_(this), operatorAddressApprovals_(this) {
  this->name_.commit();
  this->symbol_.commit();
  this->registerContractFunctions();
}

ERC721::ERC721(
  const std::string &derivedTypeName,
  const std::string &erc721name, const std::string &erc721symbol_,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, derivedTypeName, address, creator, chainId, db), name_(this, erc721name),
    symbol_(this, erc721symbol_), owners_(this), balances_(this), tokenApprovals_(this), operatorAddressApprovals_(this) {
  this->name_.commit();
  this->symbol_.commit();
  this->registerContractFunctions();
}

ERC721::~ERC721() {
  DBBatch batchedOperations;

  this->db_->put(std::string("name_"), name_.get(), this->getDBPrefix());
  this->db_->put(std::string("symbol_"), symbol_.get(), this->getDBPrefix());

  for (auto it = owners_.cbegin(), end = owners_.cend(); it != end; ++it) {
    // key: uint -> value: Address
    batchedOperations.push_back(Utils::uintToBytes(it->first), it->second.get(), this->getNewPrefix("owners_"));
  }

  for (auto it = balances_.cbegin(), end = balances_.cend(); it != end; ++it) {
    // key: Address -> value: uint
    batchedOperations.push_back(it->first.get(), Utils::uintToBytes(it->second), this->getNewPrefix("balances_"));
  }

  for (auto it = tokenApprovals_.cbegin(), end = tokenApprovals_.cend(); it != end; ++it) {
    // key: uint -> value: Address
    batchedOperations.push_back(Utils::uintToBytes(it->first), it->second.get(), this->getNewPrefix("tokenApprovals_"));
  }

  for (auto it = operatorAddressApprovals_.cbegin(); it != operatorAddressApprovals_.cend(); ++it) {
    for (auto it2 = it->second.cbegin(); it2 != it->second.cend(); ++it2) {
      // key: address -> value: address + bool (1 byte)
      const auto& key = it->first.get();
      Bytes value = it2->first.asBytes();
      value.insert(value.end(), char(it2->second));
      batchedOperations.push_back(key, value, this->getNewPrefix("operatorAddressApprovals_"));
    }
  }
}

void ERC721::registerContractFunctions() {
  this->registerContract();
  this->registerMemberFunction("name", &ERC721::name, this);
  this->registerMemberFunction("symbol", &ERC721::symbol, this);
  this->registerMemberFunction("balanceOf", &ERC721::balanceOf, this);
  this->registerMemberFunction("ownerOf", &ERC721::ownerOf, this);
  this->registerMemberFunction("approve", &ERC721::approve, this);
  this->registerMemberFunction("getApproved", &ERC721::getApproved, this);
  this->registerMemberFunction("setApprovalForAll", &ERC721::setApprovalForAll, this);
  this->registerMemberFunction("isApprovedForAll", &ERC721::isApprovedForAll, this);
  this->registerMemberFunction("transferFrom", &ERC721::transferFrom, this);
}

Address ERC721::ownerOf_(const uint256_t& tokenId) const {
  auto it = this->owners_.find(tokenId);
  if (it == this->owners_.end()) {
    return Address();
  }
  return it->second;
}

Address ERC721::getApproved_(const uint256_t& tokenId) const {
  auto it = this->tokenApprovals_.find(tokenId);
  if (it == this->tokenApprovals_.end()) {
    return Address();
  }
  return it->second;
}

Address ERC721::update_(const Address& to, const uint256_t& tokenId, const Address& auth) {
  Address from = this->ownerOf_(tokenId);
  if (auth) {
    this->checkAuthorized_(from, auth, tokenId);
  }
  if (from) {
    this->tokenApprovals_[tokenId] = Address();
    this->balances_[from]--;
  }
  if (to) {
    this->balances_[to]++;
  }
  this->owners_[tokenId] = to;
  return from;
}

void ERC721::checkAuthorized_(const Address& owner, const Address& spender, const uint256_t& tokenId) const {
  if (!this->isAuthorized_(owner, spender, tokenId)) {
    if (owner) {
      throw std::runtime_error("ERC721::checkAuthorized_: Not authorized");
    }
    throw std::runtime_error("ERC721::checkAuthorized_: inexistent token");
  }
}

bool ERC721::isAuthorized_(const Address& owner, const Address& spender, const uint256_t& tokenId) const {
  if (spender == owner) { return true; }
  if (spender == Address()) { return false; }
  if (this->isApprovedForAll(owner, spender)) { return true; }
  if (this->getApproved_(tokenId) == spender) { return true; }
  return false;
}

void ERC721::mint_(const Address& to, const uint256_t& tokenId) {
  if (to == Address()) {
    throw std::runtime_error("ERC721::mint_: mint to the zero address");
  }
  Address prevOwner = this->update_(to, tokenId, Address());
}

void ERC721::burn_(const uint256_t& tokenId) {
  Address prevOwner = this->update_(Address(), tokenId, Address());
  if (prevOwner == Address()) {
    throw std::runtime_error("ERC721::burn_: inexistent token");
  }
}

void ERC721::transfer_(const Address& from, const Address& to, const uint256_t& tokenId) {
  if (to == Address()) {
    throw std::runtime_error("ERC721::transfer_: transfer to the zero address");
  }

  Address prevOwner = this->update_(to, tokenId, Address());
  if (prevOwner == Address()) {
    throw std::runtime_error("ERC721::transfer_: inexistent token");
  } else if (prevOwner != from) {
    throw std::runtime_error("ERC721::transfer_: incorrect owner");
  }
}

Address ERC721::approve_(const Address& to, const uint256_t& tokenId, const Address& auth) {
  Address owner = this->ownerOf(tokenId);

  if (auth != Address() && owner != auth && !this->isApprovedForAll(owner, auth)) {
    throw std::runtime_error("ERC721::approve_: Not authorized");
  }

  this->tokenApprovals_[tokenId] = to;

  return owner;
}

std::string ERC721::name() const {
  return this->name_.get();
}

std::string ERC721::symbol() const {
  return this->symbol_.get();
}

uint256_t ERC721::balanceOf(const Address& owner) const {
  if (owner == Address()) {
    throw std::runtime_error("ERC721::balanceOf: zero address");
  }
  auto it = this->balances_.find(owner);
  if (it == this->balances_.end()) {
    return 0;
  }
  return it->second;
}

Address ERC721::ownerOf(const uint256_t& tokenId) const {
  Address owner = this->ownerOf_(tokenId);
  if (owner == Address()) {
    throw std::runtime_error("ERC721::ownerOf: inexistent token");
  }
  return owner;
}

std::string ERC721::tokenURI(const uint256_t& tokenId) const {
  this->requireMinted_(tokenId);
  return this->baseURI_() + tokenId.str();
}

void ERC721::approve(const Address& to, const uint256_t& tokenId) {
  approve_(to, tokenId, this->getCaller());
}

Address ERC721::getApproved(const uint256_t &tokenId) const {
  this->requireMinted_(tokenId);
  return this->getApproved_(tokenId);
}

void ERC721::setApprovalForAll(const Address& operatorAddress, const bool& approved) {
  this->setApprovalForAll_(this->getCaller(), operatorAddress, approved);
}

void ERC721::setApprovalForAll_(const Address& owner, const Address& operatorAddress, bool approved) {
  if (operatorAddress == Address()) {
    throw std::runtime_error("ERC721::setApprovalForAll_: zero address");
  }
  this->operatorAddressApprovals_[owner][operatorAddress] = approved;
}

void ERC721::requireMinted_(const uint256_t& tokenId) const {
  if (this->ownerOf_(tokenId) == Address()) {
    throw std::runtime_error("ERC721::requireMinted_: inexistent token");
  }
}

bool ERC721::isApprovedForAll(const Address& owner, const Address& operatorAddress) const {
  auto it = this->operatorAddressApprovals_.find(owner);
  if (it == this->operatorAddressApprovals_.end()) {
    return false;
  }
  auto it2 = it->second.find(operatorAddress);
  if (it2 == it->second.end()) {
    return false;
  }
  return it2->second;
}

void ERC721::transferFrom(const Address& from, const Address& to, const uint256_t& tokenId) {
  if (to == Address()) {
    throw std::runtime_error("ERC721::transferFrom: transfer to the zero address");
  }
  Address prevOwner = this->update_(to, tokenId, this->getCaller());
  if (prevOwner == Address()) {
    throw std::runtime_error("ERC721::transferFrom: inexistent token");
  } else if (prevOwner != from) {
    throw std::runtime_error("ERC721::transferFrom: incorrect owner");
  }
}