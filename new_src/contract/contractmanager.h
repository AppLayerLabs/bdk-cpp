#ifndef CONTRACTMANAGER_H
#define CONTRACTMANAGER_H

#include <memory>
#include <unordered_map>

#include "contract.h"

#include "../utils/db.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"

class Contract; // Forward declaration.

/**
 * Class that holds all current contract instances in the blockchain state.
 * Responsible for deploying contracts in the chain.
 * Also acts as an access point for contracts to access each other.
 */
class ContractManager {
  private:
    /// List of currently deployed contracts.
    std::unordered_map<Address, std::unique_ptr<Contract>> contracts;

    /// Pointer to the database.
    std::unique_ptr<DB> db;

  public:
    // TODO: constructor and destructor are not implemented because we don't know how contract loading/saving will work yet

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
     * @return The contract object, or a nullptr if contract is not found.
     */
    const std::unique_ptr<Contract>& getContract(Address address) const {
      auto it = this->contracts.find(address);
      return (it != this->contracts.end()) ? it->second : std::unique_ptr<Contract>(nullptr);
    }

    /**
     * Process a transaction that calls a function from a given contract.
     * @param tx The transaction to process.
     */
    void processTx(const TxBlock& tx) const {
      try {
        this->contracts[tx.getTo()]->ethCall(tx);
      } catch (std::exception& e) {
        Utils::logToDebug(Log::contractManager, __func__, std::string("Reverted: ") + e.what());
      }
    }
};

#endif  // CONTRACTMANAGER_H
