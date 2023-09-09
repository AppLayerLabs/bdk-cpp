#include "treasurysystem.h"

TreasurySystem::TreasurySystem(
  const std::string& name_, Address _nftContractAddress,
  ContractManagerInterface &interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB> &db
) : DynamicContract(interface, "TreasurySystem", address, creator, chainId, db),
  name(this), creationBlock(this), nftContractAddress(this),
  tokens(this), accounts(this), pausableActor_(this)
{
  this->initialize(name_, _nftContractAddress); // https://stackoverflow.com/a/74711425
  registerContractFunctions();
}

TreasurySystem::TreasurySystem(
  ContractManagerInterface &interface,
  const Address& address, const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db),
  name(this), creationBlock(this), nftContractAddress(this),
  tokens(this), accounts(this), pausableActor_(this)
{
  // TODO: load tokens and accounts from DB
  this->name = Utils::bytesToString(db->get(std::string("name"), this->getDBPrefix()));
  this->creationBlock = Utils::bytesToUint256(db->get(std::string("creationBlock"), this->getDBPrefix()));
  this->nftContractAddress = Address(this->db->get(Utils::stringToBytes("nftContractAddress"), this->getDBPrefix()));
  this->pausableActor_.paused_ = Utils::fromBigEndian<bool>(this->db->get(Utils::stringToBytes("pausableActor_"), this->getDBPrefix()));
  registerContractFunctions();
}

TreasurySystem::~TreasurySystem() {
  // TODO: save tokens and accounts to DB
  DBBatch batchedOperations;
  batchedOperations.push_back(Utils::stringToBytes("name"), Utils::stringToBytes(this->name.get()), this->getDBPrefix());
  batchedOperations.push_back(Utils::stringToBytes("creationBlock"), Utils::uint256ToBytes(this->creationBlock.get()), this->getDBPrefix());
  batchedOperations.push_back(Utils::stringToBytes("nftContractAddress"), this->nftContractAddress.get().view_const(), this->getDBPrefix());
  batchedOperations.push_back(Utils::stringToBytes("pausableActor_"), Utils::uintToBytes(pausableActor_.paused_.get()), this->getDBPrefix());
  this->db->putBatch(batchedOperations);
}

void TreasurySystem::registerContractFunctions() {
  this->registerMemberFunction("OPERATOR", &TreasurySystem::OPERATOR, this);
  this->registerMemberFunction("initialize", &TreasurySystem::initialize, this);
  this->registerMemberFunction("pause", &TreasurySystem::pause, this);
  this->registerMemberFunction("unpause", &TreasurySystem::unpause, this);
  this->registerMemberFunction("addOperator", &TreasurySystem::addOperator, this);
  this->registerMemberFunction("revokeOperator", &TreasurySystem::revokeOperator, this);
  this->registerMemberFunction("setToken", &TreasurySystem::setToken, this);
  this->registerMemberFunction("getTokensLength", &TreasurySystem::getTokensLength, this);
  this->registerMemberFunction("getTokens", &TreasurySystem::getTokens, this);
  this->registerMemberFunction("removeToken", &TreasurySystem::removeToken, this);
  this->registerMemberFunction("setTokensStatus", &TreasurySystem::setTokensStatus, this);
  this->registerMemberFunction("getTokensStatus", &TreasurySystem::getTokensStatus, this);
  this->registerMemberFunction("createAccount", &TreasurySystem::createAccount, this);
  this->registerMemberFunction("removeAccount", &TreasurySystem::removeAccount, this);
  this->registerMemberFunction("setAccountInfo", &TreasurySystem::setAccountInfo, this);
  this->registerMemberFunction("existAccount", &TreasurySystem::existAccount, this);
  this->registerMemberFunction("getAccount", &TreasurySystem::getAccount, this);
  this->registerMemberFunction("getAccountsLength", &TreasurySystem::getAccountsLength, this);
  this->registerMemberFunction("getAccountByIndex", &TreasurySystem::getAccountByIndex, this);
  this->registerMemberFunction("linkAccountToWalletAddress", &TreasurySystem::linkAccountToWalletAddress, this);
  this->registerMemberFunction("mint", &TreasurySystem::mint, this);
  this->registerMemberFunction("burn", &TreasurySystem::burn, this);
  this->registerMemberFunction("mintToAccount", &TreasurySystem::mintToAccount, this);
  this->registerMemberFunction("burnFromAccount", &TreasurySystem::burnFromAccount, this);
  this->registerMemberFunction("transfer", &TreasurySystem::transfer, this);
  this->registerMemberFunction("getBalanceOfTokenInAccount", &TreasurySystem::getBalanceOfTokenInAccount, this);
  this->registerMemberFunction("getBalanceOfAllTokensInAccount", &TreasurySystem::getBalanceOfAllTokensInAccount, this);
  this->registerMemberFunction("assignNftsToAccount", &TreasurySystem::assignNftsToAccount, this);
  this->registerMemberFunction("deassignNftsToAccount", &TreasurySystem::deassignNftsToAccount, this);
  this->registerMemberFunction("getNftsFromAccount", &TreasurySystem::getNftsFromAccount, this);
}

