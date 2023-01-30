#ifndef CONTRACTMANAGER_H
#define CONTRACTMANAGER_H

#include <memory>
#include <unordered_map>

#include "../utils/db.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
//#include "contract.h" // TODO: circular dep

/**
 * Class that holds all current contract instances in the blockchain state.
 * Responsible for deploying contracts in the chain.
 * Also acts as an access point for contracts to access each other.
 */
class ContractManager {
  private:
    /// List of currently deployed contracts.
    std::unordered_map<Address, std::unique_ptr<Contract>> contracts;

    std::unique_ptr<DB> db; ///< Pointer to the database.

  public:
    /**
     * Constructor. Automatically loads contracts from the database and deploys them.
     * @param db Pointer to the database.
     */
    ContractManager(std::unique_ptr<DB>& db) : db(db);

    /// Destructor. Automatically saves contracts to the database before wiping them.
    ~ContractManager();

    /**
     * Get a contract object from the deployed list.
     * @param address The address where the contract is deployed.
     * @return The contract object.
     */
    std::unique_ptr<Contract>& getContract(Address address);

    /// Const-friendly overload of `getContract()`.
    const std::unique_ptr<Contract>& getContract(Address address);

    /**
     * Process a transaction that calls a function from a given contract.
     * @param tx The transaction to process.
     */
    void processTx(const TxBlock& tx);
};

#endif  // CONTRACTMANAGER_H
