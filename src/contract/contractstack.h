#ifndef STACKTRACE_H
#define STACKTRACE_H

#include <../utils/strings.h>
#include <../utils/safehash.h>
#include <../contract/event.h>

/**
 * ContractStack is a class/object required to initialize a sequence of contract executions (1 tx == 1 contract stack).
 * The ContractStack have the following responsabilities:
 * - Store original values of state variables (e.g. balance, code, nonce, evm storage, used C++ variables) etc
 */

class ContractStack {
  private:
    ankerl::unordered_dense::map<Address, Bytes, SafeHash> code_;
    ankerl::unordered_dense::map<Address, uint256_t, SafeHash> balance_;
    ankerl::unordered_dense::map<Address, uint64_t, SafeHash> nonce_;
    ankerl::unordered_dense::map<StorageKey, Hash, SafeHash> storage_;
    std::vector<Event> events_;
    std::vector<std::pair<Address,BaseContract*>> contracts_; // Contracts that have been created during the execution of the call, we need to revert them if the call reverts.
    std::vector<std::reference_wrapper<SafeBase>> usedVars_;

  public:
    inline void registerCode(const Address& addr, const Bytes& code)  {
      this->code_.try_emplace(addr, code);
    }

    inline void registerBalance(const Address& addr, const uint256_t& balance) {
      this->balance_.try_emplace(addr, balance);
    }

    inline void registerNonce(const Address& addr, const uint64_t& nonce) {
      this->nonce_.try_emplace(addr, nonce);
    }

    inline void registerStorageChange(const StorageKey& key, const Hash& value) {
      this->storage_.try_emplace(key, value);
    }

    inline void registerEvent(Event event) {
      this->events_.emplace_back(std::move(event));
    }

    inline void registerContract(const Address& addr, BaseContract* contract) {
      this->contracts_.emplace_back(addr, contract);
    }

    inline void registerVariableUse(SafeBase& var) {
      this->usedVars_.emplace_back(var);
    }

    /// Getters
    inline const ankerl::unordered_dense::map<Address, Bytes, SafeHash>& getCode() const { return this->code_; }
    inline const ankerl::unordered_dense::map<Address, uint256_t, SafeHash>& getBalance() const { return this->balance_; }
    inline const ankerl::unordered_dense::map<Address, uint64_t, SafeHash>& getNonce() const { return this->nonce_; }
    inline const ankerl::unordered_dense::map<StorageKey, Hash, SafeHash>& getStorage() const { return this->storage_; }
    inline std::vector<Event>& getEvents() { return this->events_; }
    inline const std::vector<std::pair<Address,BaseContract*>>& getContracts() const { return this->contracts_; }
    inline const std::vector<std::reference_wrapper<SafeBase>>& getUsedVars() const { return this->usedVars_; }
};


#endif // STACKTRACE_H
