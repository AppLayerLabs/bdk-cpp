/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STACKTRACE_H
#define STACKTRACE_H

// leave it in to avoid "AddressSanitizer unknown-crash" runtime errors
#include "contract.h" // core/dump.h -> utils/db.h -> utils.h -> strings.h

#include "event.h"

/**
 * ContractStack is a class/object required to initialize a sequence of contract executions (1 tx == 1 contract stack).
 * The ContractStack have the following responsabilities:
 * - Store original values of state variables (e.g. balance, code, nonce, evm storage, used C++ variables) etc
 */
class ContractStack {
  private:
    boost::unordered_flat_map<Address, Bytes, SafeHash> code_; ///< Map for address code fields.
    boost::unordered_flat_map<Address, uint256_t, SafeHash> balance_; ///< Map for address balances.
    boost::unordered_flat_map<Address, uint64_t, SafeHash> nonce_; ///< Map for address nonces.
    boost::unordered_flat_map<StorageKey, Hash, SafeHash> storage_; ///< Map for storage keys.
    std::vector<Event> events_; ///< List of contract events in the stack.
    std::vector<std::pair<Address,BaseContract*>> contracts_; ///< List of contracts that have been created during the execution of the call, so they can be reverted if the call reverts.
    std::vector<std::reference_wrapper<SafeBase>> usedVars_; ///< List of used SafeVars during the stack execution.

  public:
    /**
     * Register a new address code field in the stack.
     * @param addr The address to register.
     * @param code The code field to register.
     */
    inline void registerCode(const Address& addr, const Bytes& code) { this->code_.try_emplace(addr, code); }

    /**
     * Register a new address balance in the stack.
     * @param addr The address to register.
     * @param balance The balance to register.
     */
    inline void registerBalance(const Address& addr, const uint256_t& balance) { this->balance_.try_emplace(addr, balance); }

    /**
     * Register a new address nonce in the stack.
     * @param addr The address to register.
     * @param nonce The nonce to register.
     */
    inline void registerNonce(const Address& addr, const uint64_t& nonce) { this->nonce_.try_emplace(addr, nonce); }

    /**
     * Register a new storage key change in the stack.
     * @param key The storage key to register.
     * @param value The storage key value to register.
     */
    inline void registerStorageChange(const StorageKey& key, const Hash& value) { this->storage_.try_emplace(key, value); }

    /**
     * Register a new emitted contract event in the stack.
     * @param event The event to register.
     */
    inline void registerEvent(Event event) { this->events_.emplace_back(std::move(event)); }

    /**
     * Register a new contract creation in the stack.
     * @param addr The contract address to register.
     * @param contract The contract class to register.
     */
    inline void registerContract(const Address& addr, BaseContract* contract) { this->contracts_.emplace_back(addr, contract); }

    /**
     * Register a new used SafeVar in the stack.
     * @param var The SafeVar to register.
     */
    inline void registerVariableUse(SafeBase& var) { this->usedVars_.emplace_back(var); }

    ///@{
    /** Getter. */
    inline const boost::unordered_flat_map<Address, Bytes, SafeHash>& getCode() const { return this->code_; }
    inline const boost::unordered_flat_map<Address, uint256_t, SafeHash>& getBalance() const { return this->balance_; }
    inline const boost::unordered_flat_map<Address, uint64_t, SafeHash>& getNonce() const { return this->nonce_; }
    inline const boost::unordered_flat_map<StorageKey, Hash, SafeHash>& getStorage() const { return this->storage_; }
    inline std::vector<Event>& getEvents() { return this->events_; }
    inline const std::vector<std::pair<Address,BaseContract*>>& getContracts() const { return this->contracts_; }
    inline const std::vector<std::reference_wrapper<SafeBase>>& getUsedVars() const { return this->usedVars_; }
    ///@}
};

#endif // STACKTRACE_H
