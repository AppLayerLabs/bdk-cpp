#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include "../../utils/utils.h";
#include "../variables/safestring.h";
#include "../variables/accesscontrol.h"
#include "../variables/pausable.h"
#include "../abi.h";
#include "../dynamiccontract.h";

#include "enumerabletokens.h";
#include "enumerabletreasury.h";
#include "enumerableaccounts.h";
#include "enumerablenftattributes.h";
#include "enumerableattributes.h";

#include <utility>

class Attributes : virtual public DynamicContract, virtual public AccessControl {
  protected:
    SafeString name;
    SafeUint256 creationBlock;
    NftAttributesMap attributes;
    Pausable::PausableActor pausableActor_;
    const Hash OPERATOR_ = Hash(Hex::toBytes("523a704056dcd17bcf83bed8b68c59416dac1119be77755efe3bde0a64e46e0c"));  // keccak("OPERATOR")

    void registerContractFunctions() override;

  public:
    using ConstructorArguments = std::tuple<const std::string&>;

    const std::string E_UNKNOWN = "E_U";
    const std::string E_RECEIVE = "E_R";
    const std::string E_FALLBACK = "E_F";

    Attributes(
      const std::string& name_,
      ContractManagerInterface &interface,
      const Address& address, const Address& creator,
      const uint64_t& chainId, const std::unique_ptr<DB> &db
    );

    Attributes(
      ContractManagerInterface &interface,
      const Address& address, const std::unique_ptr<DB> &db
    );

    ~Attributes() override;

    const Hash OPERATOR() const { return this->OPERATOR_; }

    void initialize(const std::string& _name) {
      this->name = _name;
      this->creationBlock = this->getBlockHeight();
      this->grantRole(AccessControl::DEFAULT_ADMIN_ROLE(), this->getCaller());
      this->grantRole(this->OPERATOR(), this->getCaller());
    }

    void pause() {
      this->onlyRole(this->OPERATOR());
      Pausable::pause(this->pausableActor_);
    }

    void unpause() {
      this->onlyRole(this->OPERATOR());
      Pausable::unpause(this->pausableActor_);
    }

    void addOperator(Address opAdd) {
      this->onlyRole(AccessControl::DEFAULT_ADMIN_ROLE());
      this->grantRole(OPERATOR(), opAdd);
    }

    void revokeOperator(Address opAdd) {
      this->onlyRole(AccessControl::DEFAULT_ADMIN_ROLE());
      this->revokeRole(OPERATOR(), opAdd);
    }

    // NOTE: original returns void
    bool setAttribute(const std::string& uniqueId, Attribute attribute) {
      this->onlyRole(this->OPERATOR());
      return this->attributes.set(uniqueId, attribute);
    }

    // NOTE: original returns void
    bool removeAttribute(const std::string& uniqueId, const std::string& attributeName) {
      this->onlyRole(this->OPERATOR());
      return this->attributes.remove(uniqueId, attributeName);
    }

    Attribute getAttributesById(const std::string& uniqueId) {
      this->onlyRole(this->OPERATOR());
      return this->attributes.getNftAttributeById(uniqueId);
    }

    std::pair<std::string, Attribute> getAttributesByIndex(uint256_t index) {
      this->onlyRole(this->OPERATOR());
      return this->attributes.getNftAttributeByIndex(index);
    }

    uint256_t getAttributesLength() { return this->attributes.length(); }

    static void registerContract() {
      ContractReflectionInterface::registerContract<
        Attributes, ContractManagerInterface&,
        const Address&, const Address&, const uint64_t&,
        const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{"name_"},
        std::make_tuple("OPERATOR", &Attributes::OPERATOR, "view", std::vector<std::string>{}),
        std::make_tuple("initialize", &Attributes::initialize, "nonpayable", std::vector<std::string>{}),
        std::make_tuple("pause", &Attributes::pause, "nonpayable", std::vector<std::string>{}),
        std::make_tuple("unpause", &Attributes::unpause, "nonpayable", std::vector<std::string>{}),
        std::make_tuple("addOperator", &Attributes::addOperator, "nonpayable", std::vector<std::string>{"opAdd"}),
        std::make_tuple("removeOperator", &Attributes::removeOperator, "nonpayable", std::vector<std::string>{"opAdd"}),
        std::make_tuple("setAttribute", &Attributes::setAttribute, "nonpayable", std::vector<std::string>{"uniqueId", "attribute"}),
        std::make_tuple("removeAttribute", &Attributes::removeAttribute, "nonpayable", std::vector<std::string>{"uniqueId", "attributeName"}),
        std::make_tuple("getAttributesById", &Attributes::getAttributesById, "view", std::vector<std::string>{"uniqueId"}),
        std::make_tuple("getAttributesByIndex", &Attributes::getAttributesByIndex, "view", std::vector<std::string>{"index"}),
        std::make_tuple("getAttributesLength", &Attributes::getAttributesLength, "view", std::vector<std::string>{}),
      );
    }
};

#endif  // ATTRIBUTES_H
