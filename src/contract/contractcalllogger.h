/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACTCALLLOGGER_H
#define CONTRACTCALLLOGGER_H

#include <vector>
#include <unordered_map>

#include "../utils/safehash.h"
#include "../utils/strings.h"
#include "../utils/utils.h"

#include "contract.h"

// Forward declarations.
class ContractManager;
class ContractLocals;

/// Class for managing contract nested call chains and their temporary data.
class ContractCallLogger {
  private:
    /// Pointer back to the contract manager.
    ContractManager& manager_;

    /**
     * Temporary map of balances within the chain.
     * Used during callContract with a payable function.
     * Cleared after the callContract function commited to the state on the end
     * of callContract(TxBlock&) if everything was succesful.
     */
    std::unordered_map<Address, uint256_t, SafeHash> balances_;

    /**
     * Temporary list of variables used by the current contract nested call chain.
     * Acts as a buffer for atomic commit/revert operations on SafeVariables.
     * Only holds variables for *one* nested call at a time. This means that
     * once a given nested call chain ends, all variables currently in this
     * list are either commited or reverted entirely, then the list itself
     * is cleaned up so it can hold the variables of the next nested call.
     */
    std::vector<std::reference_wrapper<SafeBase>> usedVars_;

    /// Indicates whether the current call should be committed or not during logger destruction.
    bool commitCall_ = false;

    /// Commit all used SafeVariables registered in the list.
    void commit();

    /// Revert all used SafeVariables registered in the list.
    void revert();

  public:
    /**
     * Constructor.
     * @param manager Pointer back to the contract manager.
     */
    ContractCallLogger(ContractManager& manager);

    /// Destructor. Clears recently created contracts, altered balances and used SafeVariables.
    ~ContractCallLogger();

    /// Getter for `balances`.
    std::unordered_map<Address, uint256_t, SafeHash>& getBalances() { return this->balances_; }

    /**
     * Get a given balance value of a given address.
     * @param add The address to get the balance of.
     * @return The current balance for the address.
     */
    uint256_t getBalanceAt(const Address& add) { return (this->hasBalance(add)) ? this->balances_[add] : 0; }

    /**
     * Set a given balance for a given address.
     * @param add The address to set a balance to.
     * @param value The balance value to set.
     */
    inline void setBalanceAt(const Address& add, uint256_t value) { this->balances_[add] = value; }

    /**
     * Set the local variables for a given contract (origin, caller, value)
     * @param contract The contract to set the local variables for.
     * @param origin The origin address to set.
     * @param caller The caller address to set.
     * @param value The value to set.
     */
    inline void setContractVars(ContractLocals* contract, const Address& origin, const Address& caller, const uint256_t value) {
      contract->origin_ = origin;
      contract->caller_ = caller;
      contract->value_ = value;
    }

    /**
     * Add a given balance value to a given address.
     * @param to The address to add balance to.
     * @param value The balance value to add.
     */
    inline void addBalance(const Address& to, const uint256_t value) { this->balances_[to] += value; }

    /**
     * Subtract a given balance value to a given address.
     * @param to The address to subtract balance to.
     * @param value The balance value to subtract.
     */
    inline void subBalance(const Address& to, const uint256_t value) { this->balances_[to] -= value; }

    /**
     * Check if a given address is registered in the balances map.
     * @param add The address to check.
     * @return `true` if an entry exists for the address, `false` otherwise.
     */
    inline bool hasBalance(const Address& add) const { return this->balances_.contains(add); }

    /**
     * Add a SafeVariable to the list of used variables.
     * @param var The variable to add to the list.
     */
    inline void addUsedVar(SafeBase& var) { this->usedVars_.emplace_back(var); }

    /// Tell the state that the current call should be committed on the destructor.
    inline void shouldCommit() { this->commitCall_ = true; }
};

#endif  // CONTRACTCALLLOGGER_H
