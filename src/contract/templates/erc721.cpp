#include "erc721.h"


ERC721::ERC721(ContractManagerInterface& interface, const Address& address, const std::unique_ptr<DB>& db)
  : DynamicContract(interface, address, db), _name(this), _symbol(this), _owners(this), _balances(this), _tokenApprovals(this), _operatorApprovals(this) {

  this->_name = Utils::bytesToString(db->get(std::string("_name"), this->getDBPrefix()));
  this->_symbol = Utils::bytesToString(db->get(std::string("_symbol"), this->getDBPrefix()));

  auto owners = db->getBatch(this->getNewPrefix("_owners"));
  for (const auto& dbEntry : owners) {
    BytesArrView valueView(dbEntry.value);
    this->_owners[Utils::fromBigEndian<uint256_t>(dbEntry.key)] = Address(valueView.subspan(0, 20));
  }

  auto balances = db->getBatch(this->getNewPrefix("_balances"));
  for (const auto& dbEntry : balances) {
    this->_balances[Address(dbEntry.key)] = Utils::fromBigEndian<uint256_t>(dbEntry.value);
  }

  auto approvals = db->getBatch(this->getNewPrefix("_tokenApprovals"));
  for (const auto& dbEntry : approvals) {
    this->_tokenApprovals[Utils::fromBigEndian<uint256_t>(dbEntry.key)] = Address(dbEntry.value);
  }

  auto operatorApprovals = db->getBatch(this->getNewPrefix("_operatorApprovals"));
  for (const auto& dbEntry : operatorApprovals) {
    BytesArrView valueView(dbEntry.value);
    this->_operatorApprovals[Address(dbEntry.key)][Address(valueView.subspan(0, 20))] = valueView[20];
  }

  this->registerContractFunctions();
}

ERC721::ERC721(
  const std::string &erc721_name, const std::string &erc721_symbol,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "ERC721", address, creator, chainId, db), _name(this, erc721_name),
  _symbol(this, erc721_symbol), _owners(this), _balances(this), _tokenApprovals(this), _operatorApprovals(this) {
  this->_name.commit();
  this->_symbol.commit();
  this->registerContractFunctions();
}

ERC721::ERC721(
  const std::string &derivedTypeName,
  const std::string &erc721_name, const std::string &erc721_symbol,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, derivedTypeName, address, creator, chainId, db), _name(this, erc721_name),
    _symbol(this, erc721_symbol), _owners(this), _balances(this), _tokenApprovals(this), _operatorApprovals(this) {
  this->_name.commit();
  this->_symbol.commit();
  this->registerContractFunctions();
}

ERC721::~ERC721() {
  DBBatch batchedOperations;

  this->db->put(std::string("_name"), _name.get(), this->getDBPrefix());
  this->db->put(std::string("_symbol"), _symbol.get(), this->getDBPrefix());

  for (auto it = _owners.cbegin(), end = _owners.cend(); it != end; ++it) {
    // key: uint -> value: Address
    batchedOperations.push_back(Utils::uintToBytes(it->first), it->second.get(), this->getNewPrefix("_owners"));
  }

  for (auto it = _balances.cbegin(), end = _balances.cend(); it != end; ++it) {
    // key: Address -> value: uint
    batchedOperations.push_back(it->first.get(), Utils::uintToBytes(it->second), this->getNewPrefix("_balances"));
  }

  for (auto it = _tokenApprovals.cbegin(), end = _tokenApprovals.cend(); it != end; ++it) {
    // key: uint -> value: Address
    batchedOperations.push_back(Utils::uintToBytes(it->first), it->second.get(), this->getNewPrefix("_tokenApprovals"));
  }

  for (auto it = _operatorApprovals.cbegin(); it != _operatorApprovals.cend(); ++it) {
    for (auto it2 = it->second.cbegin(); it2 != it->second.cend(); ++it2) {
      // key: address -> value: address + bool (1 byte)
      const auto& key = it->first.get();
      Bytes value = it2->first.asBytes();
      value.insert(value.end(), char(it2->second));
      batchedOperations.push_back(key, value, this->getNewPrefix("_operatorApprovals"));
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

Address ERC721::_ownerOf(const uint256_t& tokenId) const {
  auto it = this->_owners.find(tokenId);
  if (it == this->_owners.end()) {
    return Address();
  }
  return it->second;
}

Address ERC721::_getApproved(const uint256_t& tokenId) const {
  auto it = this->_tokenApprovals.find(tokenId);
  if (it == this->_tokenApprovals.end()) {
    return Address();
  }
  return it->second;
}

Address ERC721::_update(const Address& to, const uint256_t& tokenId, const Address& auth) {
  Address from = this->ownerOf(tokenId);
  if (auth) {
    this->_checkAuthorized(from, auth, tokenId);
  }
  if (from) {
    this->_tokenApprovals[tokenId] = Address();
    this->_balances[from]--;
  }
  if (to) {
    this->_balances[to]++;
  }
  this->_owners[tokenId] = to;
  return from;
}

void ERC721::_checkAuthorized(const Address& owner, const Address& spender, const uint256_t& tokenId) const {
  if (!this->_isAuthorized(owner, spender, tokenId)) {
    if (owner) {
      throw std::runtime_error("ERC721::_checkAuthorized: Not authorized");
    }
    throw std::runtime_error("ERC721::_checkAuthorized: inexistent token");
  }
}

bool ERC721::_isAuthorized(const Address& owner, const Address& spender, const uint256_t& tokenId) const {
  if (spender == owner) { return true; }
  if (spender == Address()) { return false; }
  if (this->isApprovedForAll(owner, spender)) { return true; }
  if (this->_getApproved(tokenId) == spender) { return true; }
  return false;
}

void ERC721::_mint(const Address& to, const uint256_t& tokenId) {
  if (to == Address()) {
    throw std::runtime_error("ERC721::_mint: mint to the zero address");
  }
  Address prevOwner = this->_update(to, tokenId, Address());
}

void ERC721::_burn(const uint256_t& tokenId) {
  Address prevOwner = this->_update(Address(), tokenId, Address());
  if (prevOwner == Address()) {
    throw std::runtime_error("ERC721::_burn: inexistent token");
  }
}

void ERC721::_transfer(const Address& from, const Address& to, const uint256_t& tokenId) {
  if (to == Address()) {
    throw std::runtime_error("ERC721::_transfer: transfer to the zero address");
  }

  Address prevOwner = this->_update(to, tokenId, Address());
  if (prevOwner == Address()) {
    throw std::runtime_error("ERC721::_transfer: inexistent token");
  } else if (prevOwner != from) {
    throw std::runtime_error("ERC721::_transfer: incorrect owner");
  }
}

Address ERC721::_approve(const Address& to, const uint256_t& tokenId, const Address& auth) {
  Address owner = this->ownerOf(tokenId);

  if (auth != Address() && owner != auth && !this->isApprovedForAll(owner, auth)) {
    throw std::runtime_error("ERC721::_approve: Not authorized");
  }

  this->_tokenApprovals[tokenId] = to;

  return owner;
}

std::string ERC721::name() const {
  return this->_name.get();
}

std::string ERC721::symbol() const {
  return this->_symbol.get();
}

uint256_t ERC721::balanceOf(const Address& owner) const {
  if (owner == Address()) {
    throw std::runtime_error("ERC721::balanceOf: zero address");
  }
  auto it = this->_balances.find(owner);
  if (it == this->_balances.end()) {
    return 0;
  }
  return it->second;
}

Address ERC721::ownerOf(const uint256_t& tokenId) const {
  Address owner = this->_ownerOf(tokenId);
  if (owner == Address()) {
    throw std::runtime_error("ERC721::ownerOf: inexistent token");
  }
  return owner;
}

std::string ERC721::tokenURI(const uint256_t& tokenId) const {
  this->_requireMinted(tokenId);
  return this->_baseURI() + tokenId.str();
}

void ERC721::approve(const Address& to, const uint256_t& tokenId) {
  _approve(to, tokenId, this->getCaller());
}

Address ERC721::getApproved(const uint256_t &tokenId) const {
  this->_requireMinted(tokenId);
  return this->_getApproved(tokenId);
}

void ERC721::setApprovalForAll(const Address& _operator, const bool& approved) {
  this->_setApprovalForAll(this->getCaller(), _operator, approved);
}

void ERC721::_setApprovalForAll(const Address& owner, const Address& _operator, bool approved) {
  if (_operator == Address()) {
    throw std::runtime_error("ERC721::_setApprovalForAll: zero address");
  }
  this->_operatorApprovals[owner][_operator] = approved;
}

void ERC721::_requireMinted(const uint256_t& tokenId) const {
  if (this->_ownerOf(tokenId) == Address()) {
    throw std::runtime_error("ERC721::_requireMinted: inexistent token");
  }
}

bool ERC721::isApprovedForAll(const Address& owner, const Address& _operator) const {
  auto it = this->_operatorApprovals.find(owner);
  if (it == this->_operatorApprovals.end()) {
    return false;
  }
  auto it2 = it->second.find(_operator);
  if (it2 == it->second.end()) {
    return false;
  }
  return it2->second;
}

void ERC721::transferFrom(const Address& from, const Address& to, const uint256_t& tokenId) {
  if (to == Address()) {
    throw std::runtime_error("ERC721::transferFrom: transfer to the zero address");
  }
  Address prevOwner = this->_update(to, tokenId, this->getCaller());
  if (prevOwner == Address()) {
    throw std::runtime_error("ERC721::transferFrom: inexistent token");
  } else if (prevOwner != from) {
    throw std::runtime_error("ERC721::transferFrom: incorrect owner");
  }
}