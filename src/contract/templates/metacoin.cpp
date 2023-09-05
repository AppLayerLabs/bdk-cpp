#include "metacoin.h"

MetaCoin::MetaCoin(
  ContractManagerInterface& interface,
  const Address& contractAddress, const std::unique_ptr<DB>& db
) : DynamicContract(interface, contractAddress, db),
  ERC20(interface, contractAddress, db),
  ERC20Snapshot(interface, contractAddress, db),
  AccessControl(interface, contractAddress, db),
  _isAbleToTransfer(this), accountsStatus(this), pausableActor_(this)
{
  /// Load from DB
  auto isAbleToTransfer = this->db->get(this->getDBPrefix(), Utils::stringToBytes("_isAbleToTransfer"));
  this->_isAbleToTransfer = bool(isAbleToTransfer[0]);
  auto _accountsStatus = this->db->getBatch(this->getNewPrefix("accountsStatus"));
  for (const auto& dbEntry : _accountsStatus) {
    BytesArrView valueView(dbEntry.value);
    this->accountsStatus.set(Address(valueView.subspan(0,20)), Utils::fromBigEndian<uint256_t>(valueView.subspan(20)));
  }
  auto paused = this->db->get(this->getDBPrefix(), Utils::stringToBytes("pausableActor_"));
  this->pausableActor_.paused_ = bool(paused[0]);
  this->registerContractFunctions();
}

MetaCoin::MetaCoin(
  ContractManagerInterface &interface,
  const Address &address, const Address &creator,
  const uint64_t &chainId, const std::unique_ptr<DB> &db
) : DynamicContract(interface, "MetaCoin", address, creator, chainId, db),
  ERC20("MetaCoin", "", "", 18, 0, interface, address, creator, chainId, db),
  ERC20Snapshot("MetaCoin", "", "", 18, 0, interface, address, creator, chainId, db),
  AccessControl("MetaCoin", interface, address, creator, chainId, db),
    _isAbleToTransfer(this), accountsStatus(this), pausableActor_(this)
     {
  this->registerContractFunctions();
}

MetaCoin::~MetaCoin() {
  DBBatch batchedOperations;
  batchedOperations.push_back(Utils::stringToBytes("_isAbleToTransfer"), Utils::uintToBytes(this->_isAbleToTransfer.get()), this->getDBPrefix());
  for (uint64_t i = 0; i < this->accountsStatus.length(); ++i) {
    auto [key, value] = accountsStatus.at(i);
    Bytes valueb = key.asBytes();
    Utils::appendBytes(valueb, Utils::uintToBytes(value));
    batchedOperations.push_back(Utils::uint64ToBytes(i), valueb, this->getNewPrefix("accountsStatus"));
  }
  batchedOperations.push_back(Utils::stringToBytes("pausableActor_"), Utils::uintToBytes(pausableActor_.paused_.get()), this->getDBPrefix());
  this->db->putBatch(batchedOperations);
}

void MetaCoin::registerContractFunctions() {
  this->registerContract();
  this->registerMemberFunction("initialize", &MetaCoin::initialize, this);
  this->registerMemberFunction("getAccountsStatusLength", &MetaCoin::getAccountsStatusLength, this);
  this->registerMemberFunction("getAccountStatusByIndex", &MetaCoin::getAccountStatusByIndex, this);
  this->registerMemberFunction("getAccountStatus", &MetaCoin::getAccountStatus, this);
  this->registerMemberFunction("setStatus", &MetaCoin::setStatus, this);
  this->registerMemberFunction("getStatus", &MetaCoin::getStatus, this);
  this->registerMemberFunction("snapshot", &MetaCoin::snapshot, this);
  this->registerMemberFunction("pause", &MetaCoin::pause, this);
  this->registerMemberFunction("unpause", &MetaCoin::unpause, this);
  this->registerMemberFunction("mint", &MetaCoin::mint, this);
  this->registerMemberFunction("burn", &MetaCoin::burn, this);

}

void MetaCoin::_update(const Address& from, const Address& to, const uint256_t& value) {
  if (this->_isAbleToTransfer || this->hasRole(OPERATOR(), this->getCaller())) {
    ERC20Snapshot::_update(from, to, value);
    auto [exists, status] = this->accountsStatus.tryGet(to);
    if (exists && (status == this->FREEZE() || status == this->BAN())) {
      throw std::runtime_error("MetaCoin: Account is frozen or banned");
    }
    ERC20::_update(from, to, value);
  } else {
    throw std::runtime_error("MetaCoin: Transfers are disabled");
  }
}

void MetaCoin::initialize(const std::string& name, const std::string& symbol, const std::vector<Address>& operators, bool isAbleToTransfer) {
  this->_name = name;
  this->_symbol = symbol;
  this->_isAbleToTransfer = isAbleToTransfer;
  this->_grantRole(this->DEFAULT_ADMIN_ROLE(), this->getCaller());
  this->_grantRole(this->OPERATOR(), this->getCaller());
  for (auto& operator_: operators) {
    this->_grantRole(this->OPERATOR(), operator_);
  }
}

uint256_t MetaCoin::getAccountsStatusLength() const {
  return this->accountsStatus.length();
}

/// std::tuple<Address,uint256_t>
BytesEncoded MetaCoin::getAccountStatusByIndex(const uint256_t& index) {
  auto [key, value] = this->accountsStatus.at(uint64_t(index));
  BytesEncoded ret;
  ret.data = ABI::Encoder({key, value}).getData();
  return ret;
}

/// std::tuple<bool, uint256_t>
BytesEncoded MetaCoin::getAccountStatus(const Address& account) {
  auto [exists, value] = this->accountsStatus.tryGet(account);
  BytesEncoded ret;
  ret.data = ABI::Encoder({exists, value}).getData();
  return ret;
}

void MetaCoin::setStatus(const Address& account, const uint256_t& status) {
  this->onlyRole(OPERATOR());
  this->accountsStatus.set(account, status);
}

uint256_t MetaCoin::getStatus(const Address& account) {
  uint256_t value = this->accountsStatus.get(account);
  return value;
}

void MetaCoin::snapshot() {
  this->onlyRole(OPERATOR());
  this->_snapshot();
}

void MetaCoin::pause() {
  this->onlyRole(OPERATOR());
  Pausable::pause(this->pausableActor_);
}

void MetaCoin::unpause() {
  this->onlyRole(OPERATOR());
  Pausable::unpause(this->pausableActor_);
}

void MetaCoin::mint(const Address& to, const uint256_t& amount) {
  this->onlyRole(OPERATOR());
  ERC20::_mint(to, amount);
}

void MetaCoin::burn(const Address& from, const uint256_t& amount) {
  this->onlyRole(OPERATOR());
  ERC20::_burn(from, amount);
}

void MetaCoin::setIsAbleToTransfer(bool value) {
  this->onlyRole(OPERATOR());
  this->_isAbleToTransfer = value;
}
