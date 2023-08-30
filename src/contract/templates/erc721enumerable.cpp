#include "erc721enumerable.h"


ERC721Enumerable::ERC721Enumerable(ContractManagerInterface& interface, const Address& address, const std::unique_ptr<DB>& db)
  : ERC721(interface, address, db), _ownedTokens(this), _ownedTokensIndex(this), _allTokens(this), _allTokensIndex(this) {

  auto ownedTokensIndex = db->getBatch(this->getNewPrefix("_ownedTokensIndex"));
  for (const auto& dbEntry : ownedTokensIndex) {
    this->_ownedTokensIndex[Utils::fromBigEndian<uint256_t>(dbEntry.key)] = Utils::fromBigEndian<uint256_t>(dbEntry.value);
  }

  auto allTokensIndex = db->getBatch(this->getNewPrefix("_allTokensIndex"));
  for (const auto& dbEntry : allTokensIndex) {
    this->_allTokensIndex[Utils::fromBigEndian<uint256_t>(dbEntry.key)] = Utils::fromBigEndian<uint256_t>(dbEntry.value);
  }

  auto allTokens = db->getBatch(this->getNewPrefix("_allTokens"));
  for (const auto& dbEntry : allTokens) {
    this->_allTokens.emplace_back(Utils::fromBigEndian<uint256_t>(dbEntry.key));
  }

  auto ownedTokens = db->getBatch(this->getNewPrefix("_ownedTokens"));
  for (const auto& dbEntry : ownedTokens) {
    BytesArrView value = dbEntry.value;
    auto offset = 0;
    uint64_t firstSize = Utils::fromBigEndian<uint64_t>(value.subspan(offset, 1));
    ++offset;
    uint256_t first = Utils::fromBigEndian<uint256_t>(value.subspan(offset, firstSize));
    offset += firstSize;
    uint256_t second = Utils::fromBigEndian<uint256_t>(value.subspan(offset));
    this->_ownedTokens[Address(dbEntry.key)].emplace(first, second);
  }

  this->_ownedTokens.commit();
  this->_ownedTokensIndex.commit();
  this->_allTokensIndex.commit();
  this->registerContractFunctions();
}

ERC721Enumerable::ERC721Enumerable(
  const std::string &erc721_name, const std::string &erc721_symbol,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : ERC721("ERC721Enumerable", erc721_name, erc721_symbol, interface, address, creator, chainId, db),
    _ownedTokens(this), _ownedTokensIndex(this), _allTokens(this), _allTokensIndex(this) {
  this->registerContractFunctions();
}

ERC721Enumerable::ERC721Enumerable(
  const std::string &derivedTypeName,
  const std::string &erc721_name, const std::string &erc721_symbol,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : ERC721(derivedTypeName, erc721_name, erc721_symbol, interface, address, creator, chainId, db),
    _ownedTokens(this), _ownedTokensIndex(this), _allTokens(this), _allTokensIndex(this) {

  this->registerContractFunctions();
}

ERC721Enumerable::~ERC721Enumerable() {
  DBBatch batchedOperations;

  for (auto it = _ownedTokensIndex.cbegin(), end = _ownedTokensIndex.cend(); it != end; ++it) {
    // key: uint -> value: uint
    batchedOperations.push_back(Utils::uintToBytes(it->first), Utils::uintToBytes(it->second), this->getNewPrefix("_ownedTokensIndex"));
  }

  for (auto it = _allTokensIndex.cbegin(), end = _allTokensIndex.cend(); it != end; ++it) {
    // key: uint -> value: uint
    batchedOperations.push_back(Utils::uintToBytes(it->first), Utils::uintToBytes(it->second), this->getNewPrefix("_allTokensIndex"));
  }

  for (auto it = _allTokens.cbegin(), end = _allTokens.cend(); it != end; ++it) {
    // key: uint -> value: 0 (uint) (value is not used)
    batchedOperations.push_back(Utils::uintToBytes(*it), Utils::uintToBytes(uint32_t(0)), this->getNewPrefix("_allTokens"));
  }

  for (auto it = _ownedTokens.cbegin(); it != _ownedTokens.cend(); ++it) {
    for (auto it2 = it->second.cbegin(); it2 != it->second.cend(); ++it2) {
      // key: address -> value: sizeof(uint) + uint + uint
      Bytes value;
      value.insert(value.end(), Utils::bytesRequired(it2->first));
      Utils::appendBytes(value, Utils::uintToBytes(it2->first));
      Utils::appendBytes(value, Utils::uintToBytes(it2->second));
      batchedOperations.push_back(
        it->first.get(),
        value,
        this->getNewPrefix("_ownedTokens")
      );
    }
  }
}

void ERC721Enumerable::registerContractFunctions() {
  this->registerContract();
  this->registerMemberFunction("tokenOfOwnerByIndex", &ERC721Enumerable::tokenOfOwnerByIndex, this);
  this->registerMemberFunction("totalSupply", &ERC721Enumerable::totalSupply, this);
  this->registerMemberFunction("tokenByIndex", &ERC721Enumerable::tokenByIndex, this);
}

Address ERC721Enumerable::_update(const Address& to, const uint256_t& tokenId, const Address& auth) {
  Address prevOwner = ERC721::_update(to, tokenId, auth);
  if (prevOwner == Address()) {
    _addTokenToAllTokensEnumeration(tokenId);
  } else if (prevOwner != to) {
    _removeTokenFromOwnerEnumeration(prevOwner, tokenId);
  }
  if (to == Address()) {
    _removeTokenFromAllTokensEnumeration(tokenId);
  } else if (prevOwner != to) {
    _addTokenToOwnerEnumeration(to, tokenId);
  }
  return prevOwner;
}

void ERC721Enumerable::_addTokenToOwnerEnumeration(const Address& to, const uint256_t& tokenId) {
  uint256_t length = this->balanceOf(to) - 1;
  _ownedTokens[to][length] = tokenId;
  _ownedTokensIndex[tokenId] = length;
}

void ERC721Enumerable::_addTokenToAllTokensEnumeration(const uint256_t& tokenId) {
  _allTokensIndex[tokenId] = _allTokens.size();
  _allTokens.push_back(tokenId);
}

void ERC721Enumerable::_removeTokenFromOwnerEnumeration(const Address& from, const uint256_t& tokenId) {
  uint256_t lastTokenIndex = this->balanceOf(from);
  uint256_t tokenIndex = _ownedTokensIndex[tokenId];

  if (tokenIndex != lastTokenIndex) {
    uint256_t lastTokenId = _ownedTokens[from][lastTokenIndex];
    _ownedTokens[from][tokenIndex] = lastTokenId;
    _ownedTokensIndex[lastTokenId] = tokenIndex;
  }

  _ownedTokensIndex.erase(tokenId);
  _ownedTokens[from].erase(lastTokenIndex);
}

void ERC721Enumerable::_removeTokenFromAllTokensEnumeration(const uint256_t& tokenId) {
  uint256_t lastTokenIndex = _allTokens.size() - 1;
  uint256_t tokenIndex = _allTokensIndex[tokenId];

  uint256_t lastTokenId = _allTokens[uint64_t(lastTokenIndex)];

  _allTokens[uint64_t(tokenIndex)] = lastTokenId;
  _allTokensIndex[lastTokenId] = tokenIndex;

  _allTokensIndex.erase(tokenId);
  _allTokens.pop_back();
}

void ERC721Enumerable::_increaseBalance(const Address& account, const uint128_t& amount) {
  if (amount > 0) {
    throw std::runtime_error("ERC721Enumerable::_increaseBalance: amount must be zero");
  }
  ERC721::_increaseBalance(account, amount);
}

uint256_t ERC721Enumerable::tokenOfOwnerByIndex(const Address& owner, const uint256_t& index) const {
  if (index >= this->balanceOf(owner)) {
    throw std::runtime_error("ERC721Enumerable::tokenOfOwnerByIndex: index out of bounds");
  }
  auto it = _ownedTokens.find(owner);
  if (it == _ownedTokens.cend()) {
    throw std::runtime_error("ERC721Enumerable::tokenOfOwnerByIndex: owner not found");
  }
  auto it2 = it->second.find(index);
  if (it2 == it->second.cend()) {
    return 0;
  }
  return it2->second;
}

uint256_t ERC721Enumerable::totalSupply() const {
  return _allTokens.size();
}

uint256_t ERC721Enumerable::tokenByIndex(const uint256_t& index) const {
  if (index >= _allTokens.size()) {
    throw std::runtime_error("ERC721Enumerable::tokenByIndex: index out of bounds");
  }
  return _allTokens[uint64_t(index)];
}