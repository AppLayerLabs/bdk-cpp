#include "erc721royalty.h"


ERC721Royalty::ERC721Royalty(ContractManagerInterface& interface, const Address& address, const std::unique_ptr<DB>& db)
  : DynamicContract(interface, address, db),
    ERC721(interface, address, db),
    ERC2981(interface, address, db) {
  this->registerContractFunctions();
}

ERC721Royalty::ERC721Royalty(
  const std::string &erc721_name, const std::string &erc721_symbol,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "ERC721Royalty", address, creator, chainId, db),
    ERC721("ERC721Royalty", erc721_name, erc721_symbol, interface, address, creator, chainId, db),
    ERC2981("ERC721Royalty", interface, address, creator, chainId, db) {
  this->registerContractFunctions();
}

ERC721Royalty::ERC721Royalty(
  const std::string &derivedTypeName,
  const std::string &erc721_name, const std::string &erc721_symbol,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, derivedTypeName, address, creator, chainId, db),
    ERC721(derivedTypeName, erc721_name, erc721_symbol, interface, address, creator, chainId, db),
    ERC2981("ERC721Royalty", interface, address, creator, chainId, db) {

  this->registerContractFunctions();
}

void ERC721Royalty::registerContractFunctions() {
}

ERC721Royalty::~ERC721Royalty() { }

Address ERC721Royalty::_update(const Address &to, const uint256_t &tokenId, const Address &auth) {
  Address prevOwner;
  if (typeid(*this) == typeid(ERC721Royalty)) {
    prevOwner = ERC721::_update(to, tokenId, auth);
  } else {
    prevOwner = this->_ownerOf(tokenId);
  }
  return prevOwner;
}