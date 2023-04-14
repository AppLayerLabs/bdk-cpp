#ifndef CONTRACTMANAGER_H
#define CONTRACTMANAGER_H

#include <memory>
#include <unordered_map>
#include <shared_mutex>

#include "contract.h"
#include "abi.h"

#include "../utils/db.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"
#include "../utils/safehash.h"
#include "../utils/options.h"

/// Forward Declaration
class rdPoS;

/**
 * Addresses for the contracts that are deployed at protocol level
 * That means, these contracts are deployed at the beginning of the chain.
 * They cannot be dynamically deployed like other contracts, and they cannot be destroyed.
 * Instead, they are deployed in the constructor of State.
 * Such example of these contracts is rdPoS.
 * contract name -> contract address
 */

const std::unordered_map<std::string,Address> ProtocolContractAddresses = {
  {"rdPoS", Address(Hex::toBytes("0xb23aa52dbeda59277ab8a962c69f5971f22904cf"), true)},// Sha3("randomDeterministicProofOfStake").substr(0,20)
  {"ContractManager", Address(Hex::toBytes("0x0001cb47ea6d8b55fe44fdd6b1bdb579efb43e61"), true)} // Sha3("ContractManager").substr(0,20).
};


/**
 * Class that holds all current contract instances in the blockchain state.
 * Responsible for being the factory and deploying contracts in the chain.
 * Also acts as an access point for contracts to access each other.
 */

class ContractManager : Contract {
  private:
    /// List of currently deployed contracts.
    std::unordered_map<Address, std::unique_ptr<Contract>, SafeHash> contracts;

    /// Contracts mutex
    mutable std::shared_mutex contractsMutex;

    /// Reference back to the rdPoS contract.
    std::unique_ptr<rdPoS>& rdpos;

    /// Reference back to options
    std::unique_ptr<Options>& options;

    /// Derive a new contract address based on transaction sender + nonce.
    Address deriveContractAddress(const TxBlock& tx) const;

    /// Create a new ERC20 contract.
    /// function createNewERC20Contract(string memory name, string memory symbol, uint8 decimals, uint256 supply) public {}
    void createNewERC20Contract(const TxBlock& tx);

    /// Check if transaction can actually create a new ERC20 contract.
    /// function createNewERC20Contract(string memory name, string memory symbol, uint8 decimals, uint256 supply) public {}
    void validateCreateNewERC20Contract(const TxBlock& tx);

    /// Serialization function for
    /// function getDeployedContracts() public view returns (string[] memory, address[] memory) {}
    std::string getDeployedContracts() const;

  public:
    // TODO: constructor and destructor are not implemented because we don't know how contract loading/saving will work yet

    /**
     * Constructor. Automatically loads contracts from the database and deploys them.
     * @param db Pointer to the database.    Contract(const std::string& contractName,
     * const Address& address, const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
     */
    ContractManager(const std::unique_ptr<DB>& db, std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Options>& options);

    /// Destructor. Automatically saves contracts to the database before wiping them.
    ~ContractManager();

    /**
    * Override the default contract function call.
    * ContractManager process things in a non-standard (cannot use SafeVariables as contract creation actively writes to DB)
    */
    void ethCall(const TxBlock& tx, bool commit = false) override;

    /**
     * Override the default contract function call.
     * ContractManager process things in a non-standard (cannot use SafeVariables as contract creation actively writes to DB)
     */
    const std::string ethCall(const std::string& data) const override;

    /**
     * Process a transaction that calls a function from a given contract.
     * @param tx The transaction to process.
     */
    void callContract(const TxBlock& tx);

    /**
     * Validate a transaction that calls a function from a given contract.
     * @param tx The transaction to validate.
     * @return True if the transaction is valid, false otherwise.
     */
    bool validateCallContract(const TxBlock& tx);

    /**
     * Make a eth_call to a view function from the contract.
     * Used by RPC
     * @param tx
     * @return
     */
    std::string callContract(const Address& address, const std::string& data) const;

    /*
     * Check if a transaction calls a contract
     */
    bool isContractCall(const TxBlock& tx) const;

    /// Get list of contracts addresses and names.
    std::vector<std::pair<std::string, Address>> getContracts() const;
};

#endif  // CONTRACTMANAGER_H
