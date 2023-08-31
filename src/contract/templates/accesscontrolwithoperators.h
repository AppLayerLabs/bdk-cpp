#ifndef ACCESSCONTROLWITHOPERATORS_H
#define ACCESSCONTROLWITHOPERATORS_H

#include "accesscontrol.h"
#include "../variables/safeaddress.h"


class AccessControlWithOperators : public AccessControl {
  protected:

    /// Solidity: bytes32 public constant ADMIN_ROLE = keccak256("ADMIN_ROLE");
    const Hash _ADMIN_ROLE = Hash(Hex::toBytes("a49807205ce4d355092ef5a8a18f56e8913cf4a201fbe287825b095693c21775"));

    /// Solidity: bytes32 public constant OPERATOR_ROLE = keccak256("OPERATOR_ROLE");
    const Hash _OPERATOR_ROLE = Hash(Hex::toBytes("97667070c54ef182b0f5858b034beac1b6f3089aa2d3188bb1e8929f4fa9b929"));

    /// Solidity: address internal _adminAccount;
    SafeAddress _adminAccount;

    /// Solidity: mapping(uint256 => address) internal _operatorAccounts;
    SafeUnorderedMap<uint256_t, Address> _operatorAccounts;

    /// Solidity: uint256 internal _operatorAccountsLength;
    SafeUint_t<256> _operatorAccountsLength;

    void registerContractFunctions() override;
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
    AccessControlWithOperators(
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
    AccessControlWithOperators(
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
    AccessControlWithOperators(
      const std::string& derivedTypeName,
      ContractManagerInterface& interface,
      const Address& address, const Address& creator,
      const uint64_t& chainId, const std::unique_ptr<DB>& db
    );

    virtual ~AccessControlWithOperators() override;

    /// Solidity: function ADMIN_ROLE() public view returns (bytes32)
    Hash ADMIN_ROLE() const;

    /// Solidity: function OPERATOR_ROLE() public view returns (bytes32)
    Hash OPERATOR_ROLE() const;

    /// Solidity: function getAdminAccount() public view returns (address)
    Address getAdminAccount() const;

    /// Solidity: function isAdmin(address addr) public view returns (bool)
    bool isAdmin(const Address& addr) const;

    /// Solidity: function isOperator(address addr) public view returns (bool)
    bool isOperator(const Address& addr) const;

    /// Solidity: function getOperators() public view returns (address[] memory) {
    std::vector<Address> getOperators() const;

    /// Solidity: function addOperators(address[] memory operatorAccounts)
    void addOperators(const std::vector<Address>& operatorAccounts);

    /// Solidity: function addOperator(address operator) public onlyRole(DEFAULT_ADMIN_ROLE)
    void addOperator(const Address& operatorAccount);

    /// Solidity: function removeOperator(address operator)
    void removeOperator(const Address& operatorAccount);

    static void registerContract() {
      ContractReflectionInterface::registerContract<
        AccessControlWithOperators, ContractManagerInterface&,
        const Address&, const Address&, const uint64_t&,
        const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{},
        std::make_tuple("ADMIN_ROLE", &AccessControlWithOperators::ADMIN_ROLE, "view", std::vector<std::string>{}),
        std::make_tuple("OPERATOR_ROLE", &AccessControlWithOperators::OPERATOR_ROLE, "view", std::vector<std::string>{}),
        std::make_tuple("getAdminAccount", &AccessControlWithOperators::getAdminAccount, "view", std::vector<std::string>{}),
        std::make_tuple("isAdmin", &AccessControlWithOperators::isAdmin, "nonpayable", std::vector<std::string>{"addr"}),
        std::make_tuple("isOperator", &AccessControlWithOperators::isOperator, "nonpayable", std::vector<std::string>{"addr"}),
        std::make_tuple("getOperators", &AccessControlWithOperators::getOperators, "nonpayable", std::vector<std::string>{}),
        std::make_tuple("addOperators", &AccessControlWithOperators::addOperators, "nonpayable", std::vector<std::string>{"operatorAccounts"}),
        std::make_tuple("addOperator", &AccessControlWithOperators::addOperator, "nonpayable", std::vector<std::string>{"operatorAccount"}),
        std::make_tuple("removeOperator", &AccessControlWithOperators::removeOperator, "nonpayable", std::vector<std::string>{"operatorAccount"})
      );
    }
};


#endif