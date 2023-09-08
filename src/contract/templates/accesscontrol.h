#ifndef ACCESSCONTROL_H
#define ACCESSCONTROL_H

#include <memory>

#include "../../utils/contractreflectioninterface.h"
#include "../../utils/db.h"
#include "../../utils/utils.h"
#include "../abi.h"
#include "../dynamiccontract.h"
#include "../variables/safeunorderedmap.h"

struct RoleData {
  std::unordered_map<Address, bool, SafeHash> members;
  Hash adminRole;
};

/// Template for an AccessControl contract.
/// Roughly based on the OpenZeppelin implementation.
class AccessControl : virtual public DynamicContract {
  protected:

    /// Solidity: mapping(bytes32 => RoleData) private _roles;
    SafeUnorderedMap<Hash, RoleData> _roles;

    /// Solidity: bytes32 public constant DEFAULT_ADMIN_ROLE = 0x00;
    const Hash _DEFAULT_ADMIN_ROLE = Hash();

    /// Solidity: modifier onlyRole(bytes32 role)
    void onlyRole(const Hash& role) const;

    /// Solidity: function _checkRole(bytes32 role) internal view virtual
    void _checkRole(const Hash& role) const;

    /// Solidity: function _checkRole(bytes32 role, address account) internal view virtual
    void _checkRole(const Hash& role, const Address& account) const;

    /// Solidity: function _setupRole(bytes32 role, address account) internal virtual
    void _setupRole(const Hash& role, const Address& account);

    /// Solidity: function _setRoleAdmin(bytes32 role, bytes32 adminRole) internal virtual
    void _setRoleAdmin(const Hash& role, const Hash& adminRole);

    /// Solidity: function _grantRole(bytes32 role, address account) internal virtual
    void _grantRole(const Hash& role, const Address& account);

    /// Solidity: function _revokeRole(bytes32 role, address account) internal virtual
    void _revokeRole(const Hash& role, const Address& account);

    virtual void registerContractFunctions() override;

  public:
    /**
     * ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
     */
    using ConstructorArguments = std::tuple<>;

    /**
     * Constructor for loading contract from DB.
     * @param interface Reference to the contract manager interface.
     * @param contractAddress The address where the contract will be deployed.
     * @param db Reference pointer to the database object.
     */
    AccessControl(
      ContractManagerInterface& interface,
      const Address& contractAddress, const std::unique_ptr<DB>& db
    );

    /**
     * Constructor for building a new contract from scratch.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain id of the contract.
     * @param db Reference pointer to the database object.
     */
    AccessControl(
      ContractManagerInterface& interface,
      const Address& address, const Address& creator,
      const uint64_t& chainId, const std::unique_ptr<DB>& db
    );

    /**
     * Construtor for building a new contract from scratch with a derived type name.
     * @param derivedTypeName The name of the derived type.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain id of the contract.
     * @param db Reference pointer to the database object.
     */
    AccessControl(
      const std::string& derivedTypeName,
      ContractManagerInterface& interface,
      const Address& address, const Address& creator,
      const uint64_t& chainId, const std::unique_ptr<DB>& db
    );

    virtual ~AccessControl() override;

    Hash DEFAULT_ADMIN_ROLE() const;

    /// Solidity: function hasRole(bytes32 role, address account) public view virtual override returns (bool)
    virtual bool hasRole(const Hash& role, const Address& account) const;

    /// Solidity: function getRoleAdmin(bytes32 role) public view virtual override returns (bytes32)
    virtual Hash getRoleAdmin(const Hash& role) const;

    /// Solidity: function grantRole(bytes32 role, address account) public virtual override onlyRole(getRoleAdmin(role))
    virtual void grantRole(const Hash& role, const Address& account);

    /// Solidity: function revokeRole(bytes32 role, address account) public virtual override onlyRole(getRoleAdmin(role))
    virtual void revokeRole(const Hash& role, const Address& account);

    /// Solidity: function renounceRole(bytes32 role, address account) public virtual override
    virtual void renounceRole(const Hash& role, const Address& account);

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContract<
        AccessControl, ContractManagerInterface&,
        const Address&, const Address&, const uint64_t&,
        const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{},
        std::make_tuple("DEFAULT_ADMIN_ROLE", &AccessControl::DEFAULT_ADMIN_ROLE, "view", std::vector<std::string>{}),
        std::make_tuple("hasRole", &AccessControl::hasRole, "view", std::vector<std::string>{"role", "account"}),
        std::make_tuple("getRoleAdmin", &AccessControl::getRoleAdmin, "view", std::vector<std::string>{"role"}),
        std::make_tuple("withdraw", &AccessControl::grantRole, "nonpayable", std::vector<std::string>{"token", "value"}),
        std::make_tuple("transferTo", &AccessControl::revokeRole, "nonpayable", std::vector<std::string>{"token", "to", "value"}),
        std::make_tuple("deposit", &AccessControl::renounceRole, "nonpayable", std::vector<std::string>{"token", "value"})
      );
    }
};

#endif  // ACCESSCONTROL_H