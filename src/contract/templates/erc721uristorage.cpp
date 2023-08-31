#include "erc721uristorage.h"


ERC721URIStorage::ERC721URIStorage(ContractManagerInterface& interface, const Address& address, const std::unique_ptr<DB>& db)
  : DynamicContract(interface, address, db),
    ERC721(interface, address, db),
    _tokenURIs(this) {

  auto tokenURIs = db->getBatch(this->getNewPrefix("_tokenURIs"));
  for (const auto& dbEntry : tokenURIs) {
    this->_tokenURIs[Utils::fromBigEndian<uint256_t>(dbEntry.key)] = Utils::bytesToString(dbEntry.value);
  }
  this->registerContractFunctions();
}

ERC721URIStorage::ERC721URIStorage(
  const std::string &erc721_name, const std::string &erc721_symbol,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "ERC721URIStorage", address, creator, chainId, db),
    ERC721("ERC721URIStorage", erc721_name, erc721_symbol, interface, address, creator, chainId, db),
    _tokenURIs(this) {
  this->registerContractFunctions();
}

ERC721URIStorage::ERC721URIStorage(
  const std::string &derivedTypeName,
  const std::string &erc721_name, const std::string &erc721_symbol,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, derivedTypeName, address, creator, chainId, db),
    ERC721(derivedTypeName, erc721_name, erc721_symbol, interface, address, creator, chainId, db),
    _tokenURIs(this) {

  this->registerContractFunctions();
}

ERC721URIStorage::~ERC721URIStorage() {
  DBBatch batchedOperations;

  for (auto it = this->_tokenURIs.cbegin(); it != this->_tokenURIs.cend(); ++it) {
    batchedOperations.push_back(
      Utils::uintToBytes(it->first),
      Utils::stringToBytes(it->second),
      this->getNewPrefix("_tokenURIs")
    );
  }

  this->db->putBatch(batchedOperations);
}

void ERC721URIStorage::registerContractFunctions() {
  this->registerContract();
  this->registerMemberFunction("tokenURI", &ERC721URIStorage::tokenURI, this);
}

void ERC721URIStorage::_setTokenURI(const uint256_t &tokenId, const SafeString &_tokenURI) {
  if (this->_ownerOf(tokenId) == Address()) {
    throw std::runtime_error("ERC721URIStorage::_setTokenURI: Token does not exist.");
  }
  this->_tokenURIs[tokenId] = _tokenURI.get();
}

Address ERC721URIStorage::_update(const Address& to, const uint256_t& tokenId, const Address& auth) {
  Address prevOwner = ERC721::_update(to, tokenId, auth);
  if (prevOwner == Address() && this->_tokenURIs.find(tokenId) != this->_tokenURIs.end()) {
    this->_tokenURIs.erase(tokenId);
  }
  return prevOwner;
}

std::string ERC721URIStorage::tokenURI(const uint256_t &tokenId) const {
  this->_requireMinted(tokenId);
  auto it = this->_tokenURIs.find(tokenId);
  std::string _tokenURI;
  if (it != this->_tokenURIs.end()) {
    _tokenURI = it->second;
  }
  std::string base = this->_baseURI();
  if (base.size() == 0) {
    return _tokenURI;
  }
  if (_tokenURI.size() > 0) {
    return base + _tokenURI;
  }
  return ERC721::tokenURI(tokenId);
}