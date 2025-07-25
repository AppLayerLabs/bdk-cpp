/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "erc721uristorage.h"

#include "../../../utils/strconv.h"

ERC721URIStorage::ERC721URIStorage(const Address& address, const DB& db)
  : DynamicContract(address, db),
    ERC721(address, db),
    _tokenURIs(this) {
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("tokenURIs_"))) {
    this->_tokenURIs[Utils::fromBigEndian<uint256_t>(dbEntry.key)] = StrConv::bytesToString(dbEntry.value);
  }
  ERC721URIStorage::registerContractFunctions();
}

ERC721URIStorage::ERC721URIStorage(
  const std::string &erc721_name, const std::string &erc721_symbol,
  const Address &address, const Address &creator, const uint64_t &chainId
) : DynamicContract("ERC721URIStorage", address, creator, chainId),
    ERC721("ERC721URIStorage", erc721_name, erc721_symbol, address, creator, chainId),
    _tokenURIs(this) {
  ERC721URIStorage::registerContractFunctions();
}

ERC721URIStorage::ERC721URIStorage(
  const std::string &derivedTypeName,
  const std::string &erc721_name, const std::string &erc721_symbol,
  const Address &address, const Address &creator, const uint64_t &chainId
) : DynamicContract(derivedTypeName, address, creator, chainId),
    ERC721(derivedTypeName, erc721_name, erc721_symbol, address, creator, chainId),
    _tokenURIs(this) {
  ERC721URIStorage::registerContractFunctions();
}

DBBatch ERC721URIStorage::dump() const {
  DBBatch batchedOperations = ERC721::dump();
  for (auto it = this->_tokenURIs.cbegin(); it != this->_tokenURIs.cend(); ++it) {
    batchedOperations.push_back(
      Utils::uintToBytes(it->first),
      StrConv::stringToBytes(it->second),
      this->getNewPrefix("tokenURIs_")
    );
  }
  return batchedOperations;
}

void ERC721URIStorage::registerContractFunctions() {
  ERC721URIStorage::registerContract();
  this->registerMemberFunctions(std::make_tuple("tokenURI", &ERC721URIStorage::tokenURI, FunctionTypes::View, this));
  this->registerInterface(Functor(UintConv::bytesToUint32(Hex::toBytes("0x49064906"))));
}

void ERC721URIStorage::setTokenURI_(const uint256_t &tokenId, const std::string &_tokenURI) {
  if (this->ownerOf_(tokenId) == Address()) {
    throw DynamicException("ERC721URIStorage::_setTokenURI: Token does not exist.");
  }
  this->_tokenURIs[tokenId] = _tokenURI;
}

Address ERC721URIStorage::update_(const Address& to, const uint256_t& tokenId, const Address& auth) {
  Address prevOwner;
  if (typeid(*this) == typeid(ERC721URIStorage)) {
    prevOwner = ERC721::update_(to, tokenId, auth);
  } else {
    prevOwner = this->ownerOf_(tokenId);
  }
  if (prevOwner == Address() && this->_tokenURIs.find(tokenId) != this->_tokenURIs.end()) {
    this->_tokenURIs.erase(tokenId);
  }
  return prevOwner;
}

std::string ERC721URIStorage::tokenURI(const uint256_t &tokenId) const {
  this->requireMinted_(tokenId);
  auto it = this->_tokenURIs.find(tokenId);
  std::string _tokenURI;
  if (it != this->_tokenURIs.cend()) _tokenURI = it->second;
  std::string base = this->baseURI_();
  if (base.size() == 0) return _tokenURI;
  if (_tokenURI.size() > 0) return base + _tokenURI;
  return ERC721::tokenURI(tokenId);
}

