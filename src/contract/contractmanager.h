#ifndef CONTRACTMANAGER_H
#define CONTRACTMANAGER_H

#include <memory>
#include <unordered_map>

#include "contract.h"

#include "../utils/db.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"
#include "../utils/safehash.h"

class Contract; // Forward declaration.

/**
 * Addresses for the contracts that are deployed at protocol level
 * That means, these contracts are deployed at the beginning of the chain.
 * They cannot be dynamically deployed like other contracts, and they cannot be destroyed.
 * Instead, they are deployed in the constructor of State.
 * Such example of these contracts is rdPoS.
 * contract name -> contract address
 */

const std::unordered_map<std::string,Address> ProtocolContractAddresses = {
  {"rdPoS", Address(Hex::toBytes("0xb23aa52dbeda59277ab8a962c69f5971f22904cf"), true)} // Sha3("randomDeterministicProofOfStake").substr(0,20)
};


/**
 * Class that holds all current contract instances in the blockchain state.
 * Responsible for deploying contracts in the chain.
 * Also acts as an access point for contracts to access each other.
 */

class ContractManager {
  private:
    /// List of currently deployed contracts.
    std::unordered_map<Address, std::unique_ptr<Contract>, SafeHash> contracts;

    /// Pointer to the database.
    const std::unique_ptr<DB>& db;

  public:
    // TODO: constructor and destructor are not implemented because we don't know how contract loading/saving will work yet

    /**
     * Constructor. Automatically loads contracts from the database and deploys them.
     * @param db Pointer to the database.
     */
    ContractManager(const std::unique_ptr<DB>& db) : db(db) {}

    /// Destructor. Automatically saves contracts to the database before wiping them.
    ~ContractManager() {};

    /**
     * Get a contract object from the deployed list.
     * @param address The address where the contract is deployed.
     * @return The contract object, or a nullptr if contract is not found.
     */
    const std::unique_ptr<Contract>& getContract(Address address) const;

    /**
     * Process a transaction that calls a function from a given contract.
     * @param tx The transaction to process.
     */
    void processTx(const TxBlock& tx);
};

#endif  // CONTRACTMANAGER_H
