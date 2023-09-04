#ifndef ERC20SNAPSHOT_H
#define ERC20SNAPSHOT_H

#include "erc20.h"
#include "../variables/counters.h"
#include "../variables/safevector.h"

/// Template for ERC20Snapshot
/// Roughly based on the OpenZeppelin implementation
class ERC20Snapshot : virtual public ERC20 {
  protected:
    struct Snapshots {
      std::vector<uint256_t> ids;
      std::vector<uint256_t> values;
    };
    struct SafeSnapshots {
      SafeVector<uint256_t> ids;
      SafeVector<uint256_t> values;
      SafeSnapshots(DynamicContract* contract) : ids(contract), values(contract) {}
    };

    /// Solidity: mapping(address => Snapshots) private _accountBalanceSnapshots;
    SafeUnorderedMap<Address, Snapshots> _accountBalanceSnapshots;
    /// Solidity: Snapshots private _totalSupplySnapshots;
    SafeSnapshots _totalSupplySnapshots;

    /// Counter for current snapshot id
    Counter _currentSnapshotId;

    /// Creates a new snapshot and returns its snapshot id
    /// Solidity: function _snapshot() internal virtual returns (uint256) {
    uint256_t _snapshot();

    /// Solidity: function _getCurrentSnapshotId() internal view virtual returns (uint256) {
    uint256_t _getCurrentSnapshotId() const;

    /// Overrides ERC20::_update to update snapshots.
    virtual void _update(const Address& from, const Address& to, const uint256_t& value) override;

    /// Solidity: function _valueAt(uint256 snapshotId, Snapshots storage snapshots) private view returns (bool, uint256)
    template <typename SnapshotType>
    std::pair<bool, uint256_t> _valueAt(const uint256_t& snapshotId, const SnapshotType& snapshots) const;

    /// Solidity: function _updateAccountSnapshot(address account) private
    void _updateAccountSnapshot(const Address& account);

    /// Solidity: function _updateTotalSupplySnapshot() private
    void _updateTotalSupplySnapshot();

    /// Solidity: function _updateSnapshot(Snapshots storage snapshots, uint256 currentValue) private
    template <typename SnapshotsType>
    void _updateSnapshot(SnapshotsType& snapshots, const uint256_t& currentValue);

    /// Solidity: function _lastSnapshotId(uint256[] storage ids) private view returns (uint256) {
    template <template <typename> class VectorType>
    uint256_t _lastSnapshotId(const VectorType<uint256_t>& ids) const;

    virtual void registerContractFunctions() override;
  public:
    /**
     * ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
     */
    using ConstructorArguments = std::tuple<const std::string&, const std::string&, const uint8_t&, const uint256_t&>;
    /**
     * Constructor for loading contract from DB.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
    */
    ERC20Snapshot(
      ContractManagerInterface& interface,
      const Address& address, const std::unique_ptr<DB>& db
    );

    /**
     * Constructor to be used when creating a new contract.
     * @param erc20_name The name of the ERC20 token.
     * @param erc20_symbol The symbol of the ERC20 token.
     * @param erc20_decimals The decimals of the ERC20 token.
     * @param mintValue The amount of tokens that will be minted.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Reference to the database object.
     */
    ERC20Snapshot(
      const std::string &erc20_name, const std::string &erc20_symbol,
      const uint8_t &erc20_decimals, const uint256_t &mintValue,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    /// Constructor for derived types!
    ERC20Snapshot(
      const std::string &derivedTypeName,
      const std::string &erc20_name, const std::string &erc20_symbol,
      const uint8_t &erc20_decimals, const uint256_t &mintValue,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    /// Destructor.
    virtual ~ERC20Snapshot() override;

    /// Solidity: function balanceOfAt(address account, uint256 snapshotId) public view virtual returns (uint256)
    uint256_t balanceOfAt(const Address& account, const uint256_t& snapshotId) const;

    /// Solidity: function totalSupplyAt(uint256 snapshotId) public view virtual returns (uint256)
    uint256_t totalSupplyAt(const uint256_t& snapshotId) const;

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContract<
        ERC20Snapshot, const std::string &, const std::string &, const uint8_t &,
        const uint256_t &, ContractManagerInterface &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"erc20_name", "erc20_symbol", "erc20_decimals", "mintValue"},
        std::make_tuple("balanceOfAt", &ERC20Snapshot::balanceOfAt, "view", std::vector<std::string>{"account", "snapshotId"}),
        std::make_tuple("totalSupplyAt", &ERC20Snapshot::totalSupplyAt, "view", std::vector<std::string>{"snapshotId"})
      );
    }
};

#endif
