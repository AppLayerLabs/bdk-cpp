/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "erc721.h"

#include "ierc721receiver.hpp"
#include "../../../utils/strconv.h"

ERC721::ERC721(const Address& address, const DB& db
) : DynamicContract(address, db), name_(this), symbol_(this),
  owners_(this), balances_(this), tokenApprovals_(this), operatorAddressApprovals_(this)
{
  this->name_ = StrConv::bytesToString(db.get(std::string("name_"), this->getDBPrefix()));
  this->symbol_ = StrConv::bytesToString(db.get(std::string("symbol_"), this->getDBPrefix()));
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("owners_"))) {
    View<Bytes> valueView(dbEntry.value);
    this->owners_[Utils::fromBigEndian<uint64_t>(dbEntry.key)] = Address(valueView.subspan(0, 20));
  }
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("balances_"))) {
    this->balances_[Address(dbEntry.key)] = Utils::fromBigEndian<uint64_t>(dbEntry.value);
  }
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("tokenApprovals_"))) {
    this->tokenApprovals_[Utils::fromBigEndian<uint64_t>(dbEntry.key)] = Address(dbEntry.value);
  }
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("operatorAddressApprovals_"))) {
    View<Bytes> keyView(dbEntry.key);
    Address owner(keyView.subspan(0, 20));
    Address operatorAddress(keyView.subspan(20));
    this->operatorAddressApprovals_[owner][operatorAddress] = dbEntry.value[0];
  }

  this->name_.commit();
  this->symbol_.commit();
  this->owners_.commit();
  this->balances_.commit();
  this->tokenApprovals_.commit();
  this->operatorAddressApprovals_.commit();

  ERC721::registerContractFunctions();

  this->name_.enableRegister();
  this->symbol_.enableRegister();
  this->owners_.enableRegister();
  this->balances_.enableRegister();
  this->tokenApprovals_.enableRegister();
  this->operatorAddressApprovals_.enableRegister();
}

ERC721::ERC721(
  const std::string &erc721name, const std::string &erc721symbol_,
  const Address &address, const Address &creator, const uint64_t &chainId
) : DynamicContract("ERC721", address, creator, chainId), name_(this, erc721name),
  symbol_(this, erc721symbol_), owners_(this), balances_(this), tokenApprovals_(this), operatorAddressApprovals_(this)
{
  this->name_.commit();
  this->symbol_.commit();
  this->owners_.commit();
  this->balances_.commit();
  this->tokenApprovals_.commit();
  this->operatorAddressApprovals_.commit();

  ERC721::registerContractFunctions();

  this->name_.enableRegister();
  this->symbol_.enableRegister();
  this->owners_.enableRegister();
  this->balances_.enableRegister();
  this->tokenApprovals_.enableRegister();
  this->operatorAddressApprovals_.enableRegister();
}

ERC721::ERC721(
  const std::string &derivedTypeName,
  const std::string &erc721name, const std::string &erc721symbol_,
  const Address &address, const Address &creator, const uint64_t &chainId
) : DynamicContract(derivedTypeName, address, creator, chainId), name_(this, erc721name),
  symbol_(this, erc721symbol_), owners_(this), balances_(this), tokenApprovals_(this), operatorAddressApprovals_(this)
{
  this->name_.commit();
  this->symbol_.commit();
  this->owners_.commit();
  this->balances_.commit();
  this->tokenApprovals_.commit();
  this->operatorAddressApprovals_.commit();

  ERC721::registerContractFunctions();

  this->name_.enableRegister();
  this->symbol_.enableRegister();
  this->owners_.enableRegister();
  this->balances_.enableRegister();
  this->tokenApprovals_.enableRegister();
  this->operatorAddressApprovals_.enableRegister();
}

void ERC721::registerContractFunctions() {
  ERC721::registerContract();
  // IERC721Metadata
  this->registerMemberFunctions(
    std::make_tuple("name", &ERC721::name, FunctionTypes::View, this),
    std::make_tuple("symbol", &ERC721::symbol, FunctionTypes::View, this),
    std::make_tuple("tokenURI", &ERC721::tokenURI, FunctionTypes::View, this)
  );
  // IERC721
  this->registerMemberFunctions(
    std::make_tuple("balanceOf", &ERC721::balanceOf, FunctionTypes::View, this),
    std::make_tuple("ownerOf", &ERC721::ownerOf, FunctionTypes::View, this),
    std::make_tuple("safeTransferFrom", static_cast<void(ERC721::*)(const Address&, const Address&, const uint256_t&, const Bytes&)>(&ERC721::safeTransferFrom), FunctionTypes::NonPayable, this),
    std::make_tuple("safeTransferFrom", static_cast<void(ERC721::*)(const Address&, const Address&, const uint256_t&)>(&ERC721::safeTransferFrom), FunctionTypes::NonPayable, this),
    std::make_tuple("transferFrom", &ERC721::transferFrom, FunctionTypes::NonPayable, this),
    std::make_tuple("approve", &ERC721::approve, FunctionTypes::NonPayable, this),
    std::make_tuple("getApproved", &ERC721::getApproved, FunctionTypes::View, this),
    std::make_tuple("setApprovalForAll", &ERC721::setApprovalForAll, FunctionTypes::NonPayable, this),
    std::make_tuple("isApprovedForAll", &ERC721::isApprovedForAll, FunctionTypes::View, this)
  );
}

Address ERC721::ownerOf_(const uint256_t& tokenId) const {
  auto it = this->owners_.find(static_cast<uint64_t>(tokenId));
  if (it == this->owners_.cend()) return Address();
  return it->second;
}

Address ERC721::getApproved_(const uint256_t& tokenId) const {
  auto it = this->tokenApprovals_.find(static_cast<uint64_t>(tokenId));
  if (it == this->tokenApprovals_.cend()) return Address();
  return it->second;
}

Address ERC721::update_(const Address& to, const uint256_t& tokenId, const Address& auth) {
  Address from = this->ownerOf_(tokenId);
  if (auth) {
    this->checkAuthorized_(from, auth, tokenId);
  }
  if (from) {
    this->tokenApprovals_[static_cast<uint64_t>(tokenId)] = Address();
    this->balances_[from]--;
  }
  if (to) {
    this->balances_[to]++;
  }
  this->owners_[static_cast<uint64_t>(tokenId)] = to;
  this->Transfer(from, to, tokenId);
  return from;
}

void ERC721::checkAuthorized_(const Address& owner, const Address& spender, const uint256_t& tokenId) const {
  if (!this->isAuthorized_(owner, spender, tokenId)) {
    if (owner) {
      throw DynamicException("ERC721::checkAuthorized_: Not authorized");
    }
    throw DynamicException("ERC721::checkAuthorized_: inexistent token");
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
    throw DynamicException("ERC721::mint_: mint to the zero address");
  }
  [[maybe_unused]] Address prevOwner = this->update_(to, tokenId, Address());
}

void ERC721::burn_(const uint256_t& tokenId) {
  Address prevOwner = this->update_(Address(), tokenId, this->getCaller());
  if (prevOwner == Address()) {
    throw DynamicException("ERC721::burn_: inexistent token");
  }
}

void ERC721::transfer_(const Address& from, const Address& to, const uint256_t& tokenId) {
  if (to == Address()) {
    throw DynamicException("ERC721::transfer_: transfer to the zero address");
  }

  Address prevOwner = this->update_(to, tokenId, this->getCaller());
  if (prevOwner == Address()) {
    throw DynamicException("ERC721::transfer_: inexistent token");
  } else if (prevOwner != from) {
    throw DynamicException("ERC721::transfer_: incorrect owner");
  }
}

Address ERC721::approve_(const Address& to, const uint256_t& tokenId, const Address& auth) {
  Address owner = this->ownerOf(tokenId);

  if (auth != Address() && owner != auth && !this->isApprovedForAll(owner, auth)) {
    throw DynamicException("ERC721::approve_: Not authorized");
  }

  this->tokenApprovals_[static_cast<uint64_t>(tokenId)] = to;
  this->Approval(owner, to, tokenId);

  return owner;
}

std::string ERC721::name() const {
  return this->name_.get();
}

std::string ERC721::symbol() const {
  return this->symbol_.get();
}

uint256_t ERC721::balanceOf(const Address& owner) const {
  if (owner == Address()) throw DynamicException("ERC721::balanceOf: zero address");
  auto it = this->balances_.find(owner);
  if (it == this->balances_.cend()) return 0;
  return it->second;
}

Address ERC721::ownerOf(const uint256_t& tokenId) const {
  Address owner = this->ownerOf_(tokenId);
  if (owner == Address()) throw DynamicException("ERC721::ownerOf: inexistent token");
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
    throw DynamicException("ERC721::setApprovalForAll_: zero address");
  }
  this->operatorAddressApprovals_[owner][operatorAddress] = approved;
  this->ApprovalForAll(owner, operatorAddress, approved);
}

void ERC721::requireMinted_(const uint256_t& tokenId) const {
  if (this->ownerOf_(tokenId) == Address()) {
    throw DynamicException("ERC721::requireMinted_: inexistent token");
  }
}

void ERC721::checkOnERC721Received_(const Address& from, const Address& to, const uint256_t& tokenId, const Bytes& data) {
  if (this->isContract(to)) {
    auto result = this->callContractFunction(to, &IERC721Receiver::onERC721Received, from, to, tokenId, data);
    if (result != IERC721Receiver::onERC721ReceiverSelector()) {
      throw DynamicException("ERC721::checkOnERC721Received_: transfer to non ERC721Receiver implementer");
    }
  }
}

bool ERC721::isApprovedForAll(const Address& owner, const Address& operatorAddress) const {
  auto it = this->operatorAddressApprovals_.find(owner);
  if (it == this->operatorAddressApprovals_.cend()) return false;
  auto it2 = it->second.find(operatorAddress);
  if (it2 == it->second.cend()) return false;
  return it2->second;
}

void ERC721::transferFrom(const Address& from, const Address& to, const uint256_t& tokenId) {
  this->transfer_(from, to, tokenId);
}

void ERC721::safeTransferFrom(const Address& from, const Address& to, const uint256_t& tokenId, const Bytes& data) {
  this->transfer_(from, to, tokenId);
  this->checkOnERC721Received_(from, to, tokenId, data);
}

void ERC721::safeTransferFrom(const Address& from, const Address& to, const uint256_t& tokenId) {
  this->safeTransferFrom(from, to, tokenId, Bytes());
}

DBBatch ERC721::dump() const {
  DBBatch dbBatch = BaseContract::dump();
  boost::unordered_flat_map<std::string, Bytes> data {
      {"name_",  StrConv::stringToBytes(name_.get())},
      {"symbol_", StrConv::stringToBytes(symbol_.get())}
  };

  for (auto it = data.cbegin(); it != data.cend(); ++it) {
    dbBatch.push_back(StrConv::stringToBytes(it->first),
                      it->second,
                      this->getDBPrefix());
  }
  for (auto it = owners_.cbegin(), end = owners_.cend(); it != end; ++it) {
    // key: uint -> value: Address
    dbBatch.push_back(Utils::uintToBytes(it->first),
                      it->second,
                      this->getNewPrefix("owners_"));
  }
  for (auto it = balances_.cbegin(), end = balances_.cend(); it != end; ++it) {
    // key: Address -> value: uint
    dbBatch.push_back(it->first,
                      Utils::uintToBytes(it->second),
                      this->getNewPrefix("balances_"));
  }
  for (auto it = tokenApprovals_.cbegin(), end = tokenApprovals_.cend(); it != end; ++it) {
    // key: uint -> value: Address
    dbBatch.push_back(Utils::uintToBytes(it->first),
                      it->second,
                      this->getNewPrefix("tokenApprovals_"));
  }
  for (auto i = operatorAddressApprovals_.cbegin(); i != operatorAddressApprovals_.cend(); ++i) {
    for (auto j = i->second.cbegin(); j != i->second.cend(); ++j) {
      // key: address + address -> bool
      Bytes key = i->first.asBytes();
      Utils::appendBytes(key, j->first.asBytes());
      Bytes value = {uint8_t(j->second)};
    }
  }
  return dbBatch;
}
