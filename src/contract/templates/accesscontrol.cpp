#include "accesscontrol.h"


AccessControl::AccessControl(
  ContractManagerInterface& interface,
  const Address& contractAddress, const std::unique_ptr<DB>& db
) : DynamicContract(interface, contractAddress, db), _roles(this) {

  auto roles = this->db->getBatch(this->getNewPrefix("_roles"));

  for (const auto& [key, value] : roles) {
    Hash role = Hash(key);
    RoleData roleData;
    BytesArrView view(value);
    for (size_t i = 0; i < value.size(); i += 21) {
      Address member = Address(view.subspan(i, 20));
      bool boolean = value[i + 20];
      roleData.members[member] = boolean;
    }
    _roles[role] = roleData;
  }

  this->registerContractFunctions();
}

AccessControl::AccessControl(
  ContractManagerInterface& interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB>& db
) : DynamicContract(interface, "AccessControl", address, creator, chainId, db), _roles(this) {
  _roles.commit();
  this->registerContractFunctions();
}

AccessControl::AccessControl(
  const std::string& derivedTypeName,
  ContractManagerInterface& interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB>& db
) : DynamicContract(interface, derivedTypeName, address, creator, chainId, db), _roles(this) {
  _roles.commit();
  this->registerContractFunctions();
}

void AccessControl::registerContractFunctions() {
  this->registerContract();
  this->registerMemberFunction("hasRole", &AccessControl::hasRole, this);
  this->registerMemberFunction("getRoleAdmin", &AccessControl::getRoleAdmin, this);
}

AccessControl::~AccessControl() {
  DBBatch batchedOperations;

  for (auto it = _roles.begin(); it != _roles.end(); ++it) {
    /// Key: Hash -> Value: RoleData
    /// RoleData: array of members (address + bool, 21 bytes * N) where N is the number of members
    /// This is not exactly efficient, but it is the most simple way to store this data.
    const auto& role = it->first;
    const auto& roleData = it->second;
    Bytes value;
    for (auto const& [roleMember, boolean] : roleData.members) {
      Utils::appendBytes(value, roleMember);
      value.insert(value.end(), static_cast<const char>(boolean));
    }
    batchedOperations.push_back(role.get(), value, this->getNewPrefix("_roles"));
  }
  this->db->putBatch(batchedOperations);
}


void AccessControl::onlyRole(const Hash &role) const {
  _checkRole(role);
}

void AccessControl::_checkRole(const Hash &role) const {
  _checkRole(role, this->getCaller());
}

void AccessControl::_checkRole(const Hash& role, const Address& account) const {
  if (!hasRole(role, account)) {
    throw std::runtime_error(std::string("AccessControl: sender: ") + account.hex().get() + " is missing the role: " + role.hex().get());
  }
}

void AccessControl::_setupRole(const Hash& role, const Address& account) {
  _grantRole(role, account);
}

void AccessControl::_setRoleAdmin(const Hash& role, const Hash& adminRole) {
  _roles[role].adminRole = adminRole;
}

void AccessControl::_grantRole(const Hash& role, const Address& account) {
  if (!hasRole(role, account)) {
    _roles[role].members[account] = true;
  }
}

void AccessControl::_revokeRole(const Hash& role, const Address& account) {
  if (hasRole(role, account)) {
    _roles[role].members[account] = false;
  }
}

Hash AccessControl::DEFAULT_ADMIN_ROLE() const {
  return _DEFAULT_ADMIN_ROLE;
}

bool AccessControl::hasRole(const Hash& role, const Address& account) const {
  auto it = _roles.find(role);
  if (it == _roles.end()) {
    return false;
  }
  auto it2 = it->second.members.find(account);
  if (it2 == it->second.members.end()) {
    return false;
  }
  return true;
}

Hash AccessControl::getRoleAdmin(const Hash& role) const {
  auto it = _roles.find(role);
  if (it == _roles.end()) {
    return Hash();
  }
  return it->second.adminRole;
}

void AccessControl::grantRole(const Hash& role, const Address& account) {
  this->onlyRole(this->getRoleAdmin(role));
  _grantRole(role, account);
}

void AccessControl::revokeRole(const Hash& role, const Address& account) {
  this->onlyRole(this->getRoleAdmin(role));
  _revokeRole(role, account);
}

void AccessControl::renounceRole(const Hash &role, const Address &account) {
  if (account != this->getCaller()) {
    throw std::runtime_error("AccessControl: can only renounce roles for self");
  }
  _revokeRole(role, account);
}
