#include "pulsar_nft.h"

PulsarNft::PulsarNft(
  ContractManagerInterface& interface,
  const Address& contractAddress, const std::unique_ptr<DB>& db
) : DynamicContract(interface, contractAddress, db),
    ERC721(interface, contractAddress, db),
    ERC721URIStorage(interface, contractAddress, db),
    ERC721Enumerable(interface, contractAddress, db),
    ERC2981(interface, contractAddress, db),
    ERC721Royalty(interface, contractAddress, db),
    AccessControl(interface, contractAddress, db),
    _tokenIdCounter(this), _baseUri(this), creationBlock_(this),
    _tokenIdIsTransferable(this), _frozenAccounts(this), _nfts(this),
    nftAttributes_(this), _users(this), pausableActor_(this),
    initialized(false) {
  this->registerContractFunctions();
  /// We have to load from the DB the variables that are dumped by the destructor
  /// in order to keep the state of the contract.
  this->_tokenIdCounter.setCounter(Utils::fromBigEndian<uint64_t>(this->db->get(Utils::stringToBytes("_tokenIdCounter"), this->getDBPrefix())));
  this->_baseUri = Utils::bytesToString(this->db->get(Utils::stringToBytes("_baseUri"), this->getDBPrefix()));
  this->creationBlock_ = Utils::fromBigEndian<uint256_t>(this->db->get(Utils::stringToBytes("creationBlock_"), this->getDBPrefix()));
  auto tokenIdIsTransferable = this->db->getBatch(this->getNewPrefix("_tokenIdIsTransferable"));
  for (auto& tokenIdIsTransferableEntry : tokenIdIsTransferable) {
    this->_tokenIdIsTransferable[Utils::fromBigEndian<uint256_t>(tokenIdIsTransferableEntry.key)] = Utils::fromBigEndian<bool>(tokenIdIsTransferableEntry.value);
  }
  auto frozenAccounts = this->db->getBatch(this->getNewPrefix("_frozenAccounts"));
  for (auto& frozenAccountsEntry : frozenAccounts) {
    this->_frozenAccounts[Address(frozenAccountsEntry.key)] = Utils::fromBigEndian<bool>(frozenAccountsEntry.value);
  }
  auto nfts = this->db->getBatch(this->getNewPrefix("_nfts"));
  uint64_t nftsIndex = 0;
  for (auto& nftsEntry : nfts) {
    uint64_t enumerableIndex = Utils::bytesToUint64(nftsEntry.key);
    if (enumerableIndex != nftsIndex) {
      throw std::runtime_error("PulsarNft::PulsarNft: invalid nfts index while trying to load from DB!");
    }
    uint8_t keySize = static_cast<uint8_t>(nftsEntry.value[0]);
    uint256_t key = Utils::fromBigEndian<uint256_t>(BytesArrView(nftsEntry.value).subspan(1, keySize));
    uint256_t value = Utils::fromBigEndian<uint256_t>(BytesArrView(nftsEntry.value).subspan(1 + keySize));
    this->_nfts.set(key, value);
    ++nftsIndex;
  }
  auto nftAttributes = this->db->getBatch(this->getNewPrefix("nftAttributes_"));
  for (auto& nftAttributesEntry : nftAttributes) {
    this->nftAttributes_[Utils::fromBigEndian<uint256_t>(nftAttributesEntry.key)] = nftAttributesEntry.value;
  }
  auto users = this->db->getBatch(this->getNewPrefix("_users"));
  for (auto& usersEntry : users) {
    Address user = Address(BytesArrView(usersEntry.value).subspan(0, 20));
    uint64_t expires = Utils::fromBigEndian<uint64_t>(BytesArrView(usersEntry.value).subspan(20));
    this->_users[Utils::fromBigEndian<uint256_t>(usersEntry.key)] = {user, expires};
  }
  this->pausableActor_.paused_ = Utils::fromBigEndian<bool>(this->db->get(Utils::stringToBytes("pausableActor_"), this->getDBPrefix()));
  this->initialized = Utils::fromBigEndian<bool>(this->db->get(Utils::stringToBytes("initialized"), this->getDBPrefix()));
}


PulsarNft::PulsarNft(
  ContractManagerInterface& interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB>& db
) : DynamicContract(interface, "PulsarNft", address, creator, chainId, db),
    ERC721("PulsarNft", "", "", interface, address, creator, chainId, db),
    ERC721URIStorage("PulsarNft", "", "", interface, address, creator, chainId, db),
    ERC2981("PulsarNft", interface, address, creator, chainId, db),
    ERC721Royalty("PulsarNft", "", "", interface, address, creator, chainId, db),
    ERC721Enumerable("PulsarNft", "", "", interface, address, creator, chainId, db),
    AccessControl("PulsarNft", interface, address, creator, chainId, db),
    _tokenIdCounter(this), _baseUri(this), creationBlock_(this),
    _tokenIdIsTransferable(this), _frozenAccounts(this), _nfts(this),
    nftAttributes_(this), _users(this), pausableActor_(this),
    initialized(false) {
  this->registerContractFunctions();
}

PulsarNft::~PulsarNft() {
  DBBatch batchedOperations;
  batchedOperations.push_back(Utils::stringToBytes("_tokenIdCounter"), Utils::uintToBytes(_tokenIdCounter.current()), this->getDBPrefix());
  batchedOperations.push_back(Utils::stringToBytes("_baseUri"), Utils::stringToBytes(_baseUri.get()), this->getDBPrefix());
  batchedOperations.push_back(Utils::stringToBytes("creationBlock_"), Utils::uintToBytes(creationBlock_.get()), this->getDBPrefix());
  for (auto it = _tokenIdIsTransferable.cbegin(); it != _tokenIdIsTransferable.cend(); ++it) {
    batchedOperations.push_back(Utils::uintToBytes(it->first), Utils::uintToBytes(it->second), this->getNewPrefix("_tokenIdIsTransferable"));
  }
  for (auto it = _frozenAccounts.cbegin(); it != _frozenAccounts.cend(); ++it) {
    batchedOperations.push_back(it->first.get(), Utils::uintToBytes(it->second), this->getNewPrefix("_frozenAccounts"));
  }
  for (uint64_t i = 0; i < _nfts.length(); ++i) {
    auto [key, value] = _nfts.at(i);
    Bytes valueb;
    valueb.insert(valueb.end(), Utils::bytesRequired(value));
    /// Key: sizeof(first) + first + second.
    Utils::appendBytes(valueb, Utils::uintToBytes(key));
    Utils::appendBytes(valueb, Utils::uintToBytes(value));
    batchedOperations.push_back(Utils::uint64ToBytes(i), valueb, Utils::stringToBytes("_nfts"));
  }
  for (auto it = nftAttributes_.cbegin(); it != nftAttributes_.cend(); ++it) {
    batchedOperations.push_back(Utils::uintToBytes(it->first), it->second, this->getNewPrefix("nftAttributes_"));
  }

  for (auto it = _users.cbegin(); it != _users.cend(); ++it) {
    Bytes value = it->second.user.asBytes();
    Utils::appendBytes(value, Utils::uintToBytes(it->second.expires));
    batchedOperations.push_back(Utils::uintToBytes(it->first), value, this->getNewPrefix("_users"));
  }
  batchedOperations.push_back(Utils::stringToBytes("pausableActor_"), Utils::uintToBytes(pausableActor_.paused_.get()), this->getDBPrefix());
  batchedOperations.push_back(Utils::stringToBytes("initialized"), Utils::uintToBytes(initialized), this->getDBPrefix());
  this->db->putBatch(batchedOperations);
}

std::string PulsarNft::_baseURI() const {
  return this->_baseUri.get();
}

std::string PulsarNft::_createUriBasedOnId(const uint256_t& baseNftId, uint256_t tokenId) {
  return std::string("&baseNftId=") + boost::lexical_cast<std::string>(baseNftId) +
  std::string("&tokenId=") + boost::lexical_cast<std::string>(tokenId) + "&t=" + boost::lexical_cast<std::string>(this->getBlockTimestamp());
}

void PulsarNft::registerContractFunctions() {
  // Register all public functions in the order of the header file
  this->registerContract("initialize", &PulsarNft::initialize, this);
  this->registerMemberFunction("OPERATOR", &PulsarNft::OPERATOR, this);
  this->registerMemberFunction("creationBlock", &PulsarNft::creationBlock, this);
  this->registerMemberFunction("nftAttributes", &PulsarNft::nftAttributes, this);
  this->registerMemberFunction("setDefaultRoyalty", &PulsarNft::setDefaultRoyalty, this);
  this->registerMemberFunction("deleteDefaultRoyalty", &PulsarNft::deleteDefaultRoyalty, this);
  this->registerMemberFunction("setBaseURI", &PulsarNft::setBaseURI, this);
  this->registerMemberFunction("pause", &PulsarNft::pause, this);
  this->registerMemberFunction("unpause", &PulsarNft::unpause, this);
  this->registerMemberFunction("setAccountFreezed", &PulsarNft::setAccountFreezed, this);
  this->registerMemberFunction("isAccountFreezed", &PulsarNft::isAccountFreezed, this);
  this->registerMemberFunction("getNftsLength", &PulsarNft::getNftsLength, this);
  this->registerMemberFunction("getNftByIndex", &PulsarNft::getNftByIndex, this);
  this->registerMemberFunction("getNft", &PulsarNft::getNft, this);
  this->registerMemberFunction("mintFor", &PulsarNft::mintFor, this);
  this->registerMemberFunction("mintNftsWithAmount", &PulsarNft::mintNftsWithAmount, this);
  this->registerMemberFunction("mintNft", &PulsarNft::mintNft, this);
  this->registerMemberFunction("burnNfts", &PulsarNft::burnNfts, this);
  this->registerMemberFunction("burnNftRanger", &PulsarNft::burnNftRanger, this);
  this->registerMemberFunction("burnNft", &PulsarNft::burnNft, this);
  this->registerMemberFunction("tokenURI", &PulsarNft::tokenURI, this);
  this->registerMemberFunction("setAttribute", &PulsarNft::setAttribute, this);
  this->registerMemberFunction("getAttributes", &PulsarNft::getAttributes, this);
  this->registerMemberFunction("removeAttributesFromTokenId", &PulsarNft::removeAttributesFromTokenId, this);
  this->registerMemberFunction("setUser", &PulsarNft::setUser, this);
  this->registerMemberFunction("userOf", &PulsarNft::userOf, this);
  this->registerMemberFunction("userExpires", &PulsarNft::userExpires, this);
}

/// Logic of _transfer + _beforeTokenTransfer + _burn + _mintNft
Address PulsarNft::_update(const Address& to, const uint256_t& tokenId, const Address& auth) {
  Pausable::requireNotPaused(this->pausableActor_);
  ERC721Enumerable::_update(to, tokenId, auth);
  ERC721URIStorage::_update(to, tokenId, auth);
  auto prevAddress = ERC721::_update(to, tokenId, auth);
  if (tokenId > this->_tokenIdCounter.current()) {
    throw std::runtime_error("PulsarNft::_update: tokenId out of bound");
  }
  auto tokenIdTransferableIt = this->_tokenIdIsTransferable.find(tokenId);
  if (tokenIdTransferableIt != this->_tokenIdIsTransferable.end()) {
    if (!tokenIdTransferableIt->second) {
      throw std::runtime_error("PulsarNft::_transfer: you cannot transfer this NFT");
    }
  }
  if (prevAddress != to && this->_users[tokenId].user != Address()) {
    this->_users.erase(tokenId);
  }
  return prevAddress;
}

void PulsarNft::initialize(const std::string &name, const std::string &symbol, const std::string &baseUri,
                           const Address &feesCollector, std::vector<Address> operators) {
  if (this->initialized) {
    throw std::runtime_error("PulsarNft::initialize: already initialized");
  }
  this->initialized = true;
  this->_grantRole(this->DEFAULT_ADMIN_ROLE(), this->getCaller());
  this->_grantRole(this->OPERATOR(), this->getCaller());
  for (auto& operator_: operators) {
    this->_grantRole(this->OPERATOR(), operator_);
  }
  this->_name = name;
  this->_symbol = symbol;
  this->_baseUri = baseUri;
  this->creationBlock_ = this->getBlockHeight();
  this->_setDefaultRoyalty(feesCollector, 100);
}

const Hash PulsarNft::OPERATOR() const {
  return this->OPERATOR_;
}

uint256_t PulsarNft::creationBlock() const {
  return this->creationBlock_.get();
}

Bytes PulsarNft::nftAttributes(const uint256_t &tokenId) const {
  Bytes ret;
  auto it = this->nftAttributes_.find(tokenId);
  if (it != this->nftAttributes_.end()) {
    ret = it->second;
  }
  return ret;
}

void PulsarNft::setDefaultRoyalty(uint96_t royalty) {
  this->onlyRole(this->OPERATOR_);
  this->_setDefaultRoyalty(this->getCaller(), royalty);
}

void PulsarNft::deleteDefaultRoyalty() {
  this->_deleteDefaultRoyalty();
}

void PulsarNft::setBaseURI(const std::string &baseUri) {
  this->onlyRole(this->OPERATOR_);
  this->_baseUri = baseUri;
}

void PulsarNft::pause() {
  this->onlyRole(this->OPERATOR_);
  Pausable::pause(this->pausableActor_);
}

void PulsarNft::unpause() {
  this->onlyRole(this->OPERATOR_);
  Pausable::unpause(this->pausableActor_);
}

void PulsarNft::setAccountFreezed(const Address &addr, bool value) {
  this->onlyRole(this->OPERATOR_);
  this->_frozenAccounts[addr] = value;
}

bool PulsarNft::isAccountFreezed(const Address &addr) const {
  auto it = this->_frozenAccounts.find(addr);
  if (it == this->_frozenAccounts.end()) {
    return false;
  }
  return it->second;
}

uint256_t PulsarNft::getNftsLength() const {
  return this->_tokenIdCounter.current();
}

/// std::tuple<uint256_t,uint256_t,address>
BytesEncoded PulsarNft::getNftByIndex(const uint256_t &index) const {
  if (index >= this->_tokenIdCounter.current()) {
    throw std::runtime_error("PulsarNft::getNftByIndex: index out of bound");
  }
  auto [tokenId, baseNftId] = this->_nfts.at(uint64_t(index));
  Address owner = this->_ownerOf(tokenId);
  BytesEncoded ret;
  ABI::Encoder encoder({tokenId, baseNftId, owner});
  ret.data = encoder.getData();
  return ret;
}

/// std::tuple<bool, uint256_t>
BytesEncoded PulsarNft::getNft(const uint256_t &_tokenId) const {
  BytesEncoded ret;
  auto [gotIt, baseNftId] = this->_nfts.tryGet(_tokenId);
  ABI::Encoder encoder({gotIt, baseNftId});
  ret.data = encoder.getData();
  return ret;
}

void PulsarNft::mintFor(const Address &user, uint256_t quantity, const std::string &mintingBlob) {
  this->onlyRole(this->OPERATOR_);
  if (quantity == 0) {
    throw std::runtime_error("PulsarNft::mintFor: quantity cannot be 0");
  }
  auto separatorPosition = mintingBlob.find_first_of(':');
  if (separatorPosition == std::string::npos) {
    throw std::runtime_error("PulsarNft::mintFor: invalid mintingBlob");
  }
  uint64_t baseNftId = boost::lexical_cast<uint64_t>(mintingBlob.substr(0, separatorPosition));
  this->mintNftsWithAmount(baseNftId, quantity, user, true);
}

void PulsarNft::mintNftsWithAmount(uint256_t baseNftId, uint256_t amount, Address to, bool isAbleToTransfer) {
  this->onlyRole(this->OPERATOR_);
  for (uint64_t i = 0; i < amount; ++i) {
    this->mintNft(to, baseNftId, isAbleToTransfer);
  }
}

void PulsarNft::mintNft(const Address &to, uint256_t baseNftId, bool isAbleToTransfer) {
  this->onlyRole(this->OPERATOR_);
  auto tokenId = this->_tokenIdCounter.current();
  this->_tokenIdCounter.increment();
  this->_mint(to, tokenId);
  std::string uri = this->_createUriBasedOnId(baseNftId, tokenId);
  this->_setTokenURI(tokenId, uri);
  this->_tokenIdIsTransferable[tokenId] = isAbleToTransfer;
  this->_nfts.set(tokenId, baseNftId);
}

void PulsarNft::burnNfts(const std::vector<uint256_t> &tokenIds) {
  this->onlyRole(this->OPERATOR_);
  for (auto tokenId : tokenIds) {
    this->burnNft(tokenId);
  }
}

void PulsarNft::burnNftRanger(uint256_t fromTokenIndex, uint256_t toTokenIndex) {
  this->onlyRole(this->OPERATOR_);
  if (fromTokenIndex > toTokenIndex) {
    throw std::runtime_error("PulsarNft::burnNftRanger: fromTokenIndex > toTokenIndex");
  }
  for (uint256_t i = fromTokenIndex; i <= toTokenIndex; ++i) {
    this->burnNft(i);
  }
}

void PulsarNft::burnNft(uint256_t tokenId) {
  if (!(this->_isAuthorized(this->_ownerOf(tokenId), this->getCaller(), tokenId) && !(this->hasRole(this->OPERATOR_, this->getCaller())))) {
    throw std::runtime_error("PulsarNft::burnNft: you are not authorized to burn this NFT");
  }
  this->_burn(tokenId);
  this->nftAttributes_.erase(tokenId);
}

std::string PulsarNft::tokenURI(const uint256_t &tokenId) const {
  return ERC721URIStorage::tokenURI(tokenId);
}

void PulsarNft::setAttribute(const uint256_t &tokenId, const Bytes &value) {
  this->onlyRole(this->OPERATOR_);
  this->nftAttributes_[tokenId] = value;
}

Bytes PulsarNft::getAttributes(const uint256_t &tokenId) const {
  auto it = this->nftAttributes_.find(tokenId);
  if (it == this->nftAttributes_.end()) {
    return Bytes();
  }
  return it->second;
}

void PulsarNft::removeAttributesFromTokenId(const uint256_t &tokenId) {
  this->onlyRole(this->OPERATOR_);
  this->nftAttributes_.erase(tokenId);
}

void PulsarNft::setUser(const uint256_t &tokenId, const Address &user, const uint64_t &expires) {
  if (!(this->_isAuthorized(this->_ownerOf(tokenId), this->getCaller(), tokenId) && !(this->hasRole(this->OPERATOR_, this->getCaller())))) {
    throw std::runtime_error("PulsarNft::burnNft: you are not authorized to burn this NFT");
  }
  auto [success, value] = this->_nfts.tryGet(tokenId);
  if (!success) {
    throw std::runtime_error("PulsarNft::setUser: tokenId does not exist");
  }
  this->_users[tokenId] = {user, expires};
}

Address PulsarNft::userOf(const uint256_t &tokenId) const {
  auto it = _users.find(tokenId);
  if (it == _users.end()) {
    return Address();
  }
  if (it->second.expires >= this->getBlockTimestamp()) {
    return it->second.user;
  } else {
    return Address();
  }
}

uint256_t PulsarNft::userExpires(const uint256_t &tokenId) const {
  auto it = _users.find(tokenId);
  if (it == _users.end()) {
    return 0;
  }
  return it->second.expires;
}
