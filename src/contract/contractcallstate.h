#ifndef CONTRACTCALLSTATE_H
#define CONTRACTCALLSTATE_H

#include <vector>
#include <unordered_map>

#include "../utils/safehash.h"
#include "../utils/strings.h"
#include "../utils/utils.h"

#include "contractmanager.h"

// Forward declarations.
class ContractManager;

/// Class for managing contract nested call chains and their temporary data.
class ContractCallState {
  private:
    /// Pointer back to the contract manager.
    ContractManager& manager;

    /**
     * Temporary map of balances within the chain.
     * Used during callContract with a payable function.
     * Cleared after the callContract function commited to the state on the end
     * of callContract(TxBlock&) if everything was succesful.
     */
    std::unordered_map<Address, uint256_t, SafeHash> balances;

    /**
     * Temporary list of variables used by the current contract nested call chain.
     * Acts as a buffer for atomic commit/revert operations on SafeVariables.
     * Only holds variables for *one* nested call at a time. This means that
     * once a given nested call chain ends, all variables currently in this
     * list are either commited or reverted entirely, then the list itself
     * is cleaned up so it can hold the variables of the next nested call.
     */
    std::vector<std::reference_wrapper<SafeBase>> usedVars;

  public:
    /**
     * Constructor.
     * @param manager Pointer back to the contract manager.
     */
    ContractCallState(ContractManager& manager) : manager(manager) {}

    /// Destructor. Clears recently created contracts, altered balances and used SafeVariables.
    ~ContractCallState() {
      this->manager.factory->clearRecentContracts();
      this->balances.clear();
      this->usedVars.clear();
    }

    /// Commit all used SafeVariables registered in the list.
    void commit() {
      for (auto rbegin = this->usedVars.rbegin(); rbegin != this->usedVars.rend(); rbegin++) {
        rbegin->get().commit();
      }
    }

    /// Revert all used SafeVariables registered in the list.
    void revert() {
      for (auto rbegin = this->usedVars.rbegin(); rbegin != this->usedVars.rend(); rbegin++) {
        rbegin->get().revert();
      }
      for (const Address& badContract : this->manager.factory->getRecentContracts()) {
        this->manager.contracts.erase(badContract); // Erase failed contract creations
      }
    }

    /// Getter for `balances`.
    std::unordered_map<Address, uint256_t, SafeHash>& getBalances() { return this->balances; }

    /**
     * Get a given balance value of a given address.
     * @param add The address to get the balance of.
     * @return The current balance for the address.
     */
    uint256_t getBalanceAt(const Address& add) { return (this->hasBalance(add)) ? this->balances[add] : 0; }

    /**
     * Set a given balance for a given address.
     * @param add The address to set a balance to.
     * @param value The balance value to set.
     */
    inline void setBalanceAt(const Address& add, uint256_t value) { this->balances[add] = value; }

    /**
     * Add a given balance value to a given address.
     * @param to The address to add balance to.
     * @param value The balance value to add.
     */
    inline void addBalance(const Address& to, const uint256_t value) { this->balances[to] += value; }

    /**
     * Subtract a given balance value to a given address.
     * @param to The address to subtract balance to.
     * @param value The balance value to subtract.
     */
    inline void subBalance(const Address& to, const uint256_t value) { this->balances[to] -= value; }

    /**
     * Check if a given address is registered in the balances map.
     * @param add The address to check.
     * @return `true` if an entry exists for the address, `false` otherwise.
     */
    inline bool hasBalance(const Address& add) const { return this->balances.contains(add); }

    /**
     * Add a SafeVariable to the list of used variables.
     * @param var The variable to add to the list.
     */
    inline void addUsedVar(SafeBase& var) { this->usedVars.emplace_back(var); }
};

#endif  // CONTRACTCALLSTATE_H
