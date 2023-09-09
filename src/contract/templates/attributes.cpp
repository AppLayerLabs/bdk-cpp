#include "attributes.h"

Attributes::Attributes(
  const std::string& name_,
  ContractManagerInterface &interface,
  const Address& address,
  const Address& creator,
  const uint64_t& chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "Attributes", address, creator, chainId, db),
  name(this), creationBlock(this), attributes(this), pausableActor_(this)
{
  this->initialize(name_); // https://stackoverflow.com/a/74711425
  registerContractFunctions();
}

Attributes::Attributes(
  ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db),
  name(this), creationBlock(this), attributes(this), pausableActor_(this)
{
  // TODO: load attributes from DB
  this->name = Utils::bytesToString(db->get(std::string("name"), this->getDBPrefix()));
  this->creationBlock = Utils::bytesToUint256(db->get(std::string("creationBlock"), this->getDBPrefix()));
  this->pausableActor_.paused_ = Utils::fromBigEndian<bool>(this->db->get(Utils::stringToBytes("pausableActor_"), this->getDBPrefix()));
  registerContractFunctions();
}

Attributes::~Attributes() {
  // TODO: save attributes to DB
  DBBatch batchedOperations;
  batchedOperations.push_back(Utils::stringToBytes("name"), Utils::stringToBytes(this->name.get()), this->getDBPrefix());
  batchedOperations.push_back(Utils::stringToBytes("creationBlock"), Utils::uint256ToBytes(this->creationBlock.get()), this->getDBPrefix());
  batchedOperations.push_back(Utils::stringToBytes("pausableActor_"), Utils::uintToBytes(pausableActor_.paused_.get()), this->getDBPrefix());
  this->db->putBatch(batchedOperations);
}

void Attributes::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("OPERATOR", &Attributes::OPERATOR, this);
  this->registerMemberFunction("initialize", &Attributes::initialize, this);
  this->registerMemberFunction("pause", &Attributes::pause, this);
  this->registerMemberFunction("unpause", &Attributes::unpause, this);
  this->registerMemberFunction("addOperator", &Attributes::addOperator, this);
  this->registerMemberFunction("revokeOperator", &Attributes::removeOperator, this);
  this->registerMemberFunction("setAttribute", &Attributes::setAttribute, this);
  this->registerMemberFunction("removeAttribute", &Attributes::removeAttribute, this);
  this->registerMemberFunction("getAttributesById", &Attributes::getAttributesById, this);
  this->registerMemberFunction("getAttributesByIndex", &Attributes::getAttributesByIndex, this);
  this->registerMemberFunction("getAttributesLength", &Attributes::getAttributesLength, this);
}

