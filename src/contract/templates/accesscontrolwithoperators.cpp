#include "accesscontrolwithoperators.h"


AccessControlWithOperators::AccessControlWithOperators(
  ContractManagerInterface& interface,
  const Address& contractAddress, const std::unique_ptr<DB>& db
) : DynamicContract(interface, contractAddress, db),
    AccessControl(interface, contractAddress, db), _adminAccount(this), _operatorAccounts(this), _operatorAccountsLength(this) {
  this->registerContractFunctions();
}

AccessControlWithOperators::AccessControlWithOperators(
  ContractManagerInterface& interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB>& db
) : DynamicContract(interface, "AccessControlWithOperators", address, creator, chainId, db),
    AccessControl("AccessControlWithOperators", interface, address, creator, chainId, db),
    _adminAccount(this), _operatorAccounts(this), _operatorAccountsLength(this) {
  this->registerContractFunctions();
}

AccessControlWithOperators::AccessControlWithOperators(
  const std::string& derivedTypeName,
  ContractManagerInterface& interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB>& db
) : DynamicContract(interface, "AccessControlWithOperators", address, creator, chainId, db),
    AccessControl(derivedTypeName, interface, address, creator, chainId, db),
    _adminAccount(this), _operatorAccounts(this), _operatorAccountsLength(this) {
  this->registerContractFunctions();
}

AccessControlWithOperators::~AccessControlWithOperators() {
  DBBatch batchedOperations;

  batchedOperations.push_back(Utils::stringToBytes("_adminAccount"), this->_adminAccount.get().get(), this->getDBPrefix());
  batchedOperations.push_back(Utils::stringToBytes("_operatorAccountsLength"), Utils::uintToBytes(this->_operatorAccountsLength.get()), this->getDBPrefix());

  for (auto it = this->_operatorAccounts.cbegin(), end = this->_operatorAccounts.cend(); it != end; ++it) {
    batchedOperations.push_back(Utils::uintToBytes(it->first), it->second.get(), this->getNewPrefix("_operatorAccounts"));
  }
  this->db->putBatch(batchedOperations);
}

void AccessControlWithOperators::registerContractFunctions() {
  this->registerContract();
  this->registerMemberFunction("ADMIN_ROLE", &AccessControlWithOperators::ADMIN_ROLE, this);
  this->registerMemberFunction("OPERATOR_ROLE", &AccessControlWithOperators::OPERATOR_ROLE, this);
  this->registerMemberFunction("getAdminAccount", &AccessControlWithOperators::getAdminAccount, this);
  this->registerMemberFunction("isAdmin", &AccessControlWithOperators::isAdmin, this);
  this->registerMemberFunction("isOperator", &AccessControlWithOperators::isOperator, this);
  this->registerMemberFunction("getOperators", &AccessControlWithOperators::getOperators, this);
  this->registerMemberFunction("addOperators", &AccessControlWithOperators::addOperators, this);
  this->registerMemberFunction("addOperator", &AccessControlWithOperators::addOperator, this);
  this->registerMemberFunction("removeOperator", &AccessControlWithOperators::removeOperator, this);
}

Hash AccessControlWithOperators::ADMIN_ROLE() const {
  return this->_ADMIN_ROLE;
}

Hash AccessControlWithOperators::OPERATOR_ROLE() const {
  return this->_OPERATOR_ROLE;
}

Address AccessControlWithOperators::getAdminAccount() const {
  return this->_adminAccount.get();
}

bool AccessControlWithOperators::isAdmin(const Address &addr) const {
  return (this->_adminAccount == addr) || this->hasRole(this->_ADMIN_ROLE, addr);
}

bool AccessControlWithOperators::isOperator(const Address& addr) const {
  for (auto it = this->_operatorAccounts.cbegin(), end = this->_operatorAccounts.cend(); it != end; ++it) {
    if (it->second == addr) {
      return true;
    }
  }
  return this->hasRole(this->_OPERATOR_ROLE, addr);
}

std::vector<Address> AccessControlWithOperators::getOperators() const {
  std::vector<Address> ret;
  /// Technically we care about the order.
  uint64_t lenght = uint64_t(this->_operatorAccountsLength.get());
  for (uint64_t i = 0; i < lenght; ++i) {
    ret.push_back(this->_operatorAccounts.at(i));
  }
  return ret;
}

void AccessControlWithOperators::addOperators(const std::vector<Address>& operatorAccounts) {
  this->onlyRole(this->_DEFAULT_ADMIN_ROLE);
  for (const auto& address : operatorAccounts) {
    if (address == Address()) {
      throw std::runtime_error("AccessControlWithOperators: cannot add zero address as operator");
      this->addOperator(address);
    }
  }
}

void AccessControlWithOperators::addOperator(const Address& operatorAccount) {
  this->onlyRole(this->_DEFAULT_ADMIN_ROLE);
  this->_grantRole(this->_OPERATOR_ROLE, operatorAccount);
  this->_operatorAccounts[uint64_t(_operatorAccountsLength.get())] = operatorAccount;
  ++_operatorAccountsLength;
}

void AccessControlWithOperators::removeOperator(const Address& operatorAccount) {
  this->onlyRole(this->_DEFAULT_ADMIN_ROLE);
  this->_revokeRole(this->_OPERATOR_ROLE, operatorAccount);
  this->_operatorAccounts.erase(uint64_t(_operatorAccountsLength.get()));
  --_operatorAccountsLength;
}


