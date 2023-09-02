#include "erc2981.h"

ERC2981::ERC2981(
  ContractManagerInterface& interface,
  const Address& contractAddress, const std::unique_ptr<DB>& db
) : DynamicContract(interface, contractAddress, db), _defaultRoyaltyInfo(this), _tokenRoyaltyInfo(this) {

  auto defaultRoyaltyInfo = this->db->get(Utils::stringToBytes("_defaultRoyaltyInfo"), this->getDBPrefix());
  BytesArrView defaultRoyaltyInfoView(defaultRoyaltyInfo);
  _defaultRoyaltyInfo.recipient = Address(defaultRoyaltyInfoView.subspan(0, 20));
  _defaultRoyaltyInfo.royaltyFraction = Utils::fromBigEndian<uint96_t>(defaultRoyaltyInfoView.subspan(20));

  auto tokenRoyaltyInfo = this->db->getBatch(this->getNewPrefix("_tokenRoyaltyInfo"));
  for (const auto& [key, value] : tokenRoyaltyInfo) {
    uint256_t tokenId = Utils::fromBigEndian<uint256_t>(key);
    BytesArrView valueView = value;
    Address recipient = Address(valueView.subspan(0, 20));
    uint96_t feeNumerator = Utils::fromBigEndian<uint96_t>(valueView.subspan(20));
    _tokenRoyaltyInfo[tokenId].recipient = recipient;
    _tokenRoyaltyInfo[tokenId].royaltyFraction = feeNumerator;
  }

  this->registerContractFunctions();
}

ERC2981::ERC2981(
  ContractManagerInterface& interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB>& db
) : DynamicContract(interface, "AccessControl", address, creator, chainId, db), _defaultRoyaltyInfo(this), _tokenRoyaltyInfo(this){
  _defaultRoyaltyInfo.recipient.commit();
  _defaultRoyaltyInfo.royaltyFraction.commit();
  _tokenRoyaltyInfo.commit();
  this->registerContractFunctions();
}

ERC2981::ERC2981(
  const std::string& derivedTypeName,
  ContractManagerInterface& interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB>& db
) : DynamicContract(interface, derivedTypeName, address, creator, chainId, db), _defaultRoyaltyInfo(this), _tokenRoyaltyInfo(this) {
  _defaultRoyaltyInfo.recipient.commit();
  _defaultRoyaltyInfo.royaltyFraction.commit();
  _tokenRoyaltyInfo.commit();
  this->registerContractFunctions();
}

ERC2981::~ERC2981() {
  DBBatch batchedOperations;
  /// key: "_defaultRoyalInfo" -> value: _defaultRoyaltyInfo.address + _defaultRoyaltyInfo.royaltyFraction
  {
    Bytes value = _defaultRoyaltyInfo.recipient.get().asBytes();
    Utils::appendBytes(value, Utils::uintToBytes(_defaultRoyaltyInfo.royaltyFraction.get()));
    batchedOperations.push_back(Utils::stringToBytes("_defaultRoyaltyInfo"), value, this->getDBPrefix());
  }

  for (auto it = this->_tokenRoyaltyInfo.cbegin(), end = this->_tokenRoyaltyInfo.cend(); it != end; ++it) {
    /// key: uint -> value: _tokenRoyaltyInfo[uint].address + _tokenRoyaltyInfo[uint].royaltyFraction
    Bytes value = it->second.recipient.asBytes();
    Utils::appendBytes(value, Utils::uintToBytes(it->second.royaltyFraction));
    batchedOperations.push_back(Utils::uintToBytes(it->first), value, this->getNewPrefix("_tokenRoyaltyInfo"));
  }

  this->db->putBatch(batchedOperations);
  _tokenRoyaltyInfo.commit();
}

void ERC2981::registerContractFunctions() {
  this->registerContract();
  this->registerMemberFunction("royaltyInfo", &ERC2981::royaltyInfo, this);
}

uint96_t ERC2981::_feeDenominator() const {
  return 10000;
}

void ERC2981::_setDefaultRoyalty(const Address& receiver, const uint96_t& feeNumerator) {
  auto denominator = this->_feeDenominator();
  if (feeNumerator > denominator) {
    throw std::runtime_error("ERC2981: invalid fee numerator");
  }
  if (receiver == Address()) {
    throw std::runtime_error("ERC2981: invalid receiver");
  }
  this->_defaultRoyaltyInfo.recipient = receiver;
  this->_defaultRoyaltyInfo.royaltyFraction = feeNumerator;
}

void ERC2981::_deleteDefaultRoyalty() {
  this->_defaultRoyaltyInfo.recipient = Address();
  this->_defaultRoyaltyInfo.royaltyFraction = 0;
}

void ERC2981::_setTokenRoyalty(const uint256_t& tokenId, const Address& receiver, const uint96_t& feeNumerator) {
  auto denominator = this->_feeDenominator();
  if (feeNumerator > denominator) {
    throw std::runtime_error("ERC2981: invalid fee numerator");
  }
  if (receiver == Address()) {
    throw std::runtime_error("ERC2981: invalid receiver");
  }
  this->_tokenRoyaltyInfo[tokenId] = {receiver, feeNumerator};
}

void ERC2981::_resetTokenRoyalty(const uint256_t& tokenId) {
  this->_tokenRoyaltyInfo.erase(tokenId);
}

BytesEncoded ERC2981::royaltyInfo(const uint256_t& tokenId, const uint256_t& salePrice) const {
  RoyaltyInfo royalty;
  auto it = this->_tokenRoyaltyInfo.find(tokenId);
  if (it == this->_tokenRoyaltyInfo.end()) {
    royalty.royaltyFraction = this->_defaultRoyaltyInfo.royaltyFraction.get();
    royalty.recipient = this->_defaultRoyaltyInfo.recipient.get();
  } else {
    royalty = it->second;
  }

  uint256_t royaltyAmount = (salePrice * royalty.royaltyFraction) / this->_feeDenominator();
  royalty.royaltyFraction = uint96_t(royaltyAmount);
  return royalty.encode();
}