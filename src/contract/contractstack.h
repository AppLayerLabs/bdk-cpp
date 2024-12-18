#ifndef STACKTRACE_H
#define STACKTRACE_H

#include <../utils/strings.h>
#include <../utils/safehash.h>
#include <../contract/event.h>
#include "contract.h"

/**
 * ContractStack is a class/object required to initialize a sequence of contract executions (1 tx == 1 contract stack).
 * The ContractStack have the following responsabilities:
 * - Store original values of state variables (e.g. balance, code, nonce, evm storage, used C++ variables) etc
 */

class ContractStack {
  private:
    std::vector<std::reference_wrapper<SafeBase>> usedVars_;

  public:
    inline void registerVariableUse(SafeBase& var) {
      this->usedVars_.emplace_back(var);
    }

    inline const std::vector<std::reference_wrapper<SafeBase>>& getUsedVars() const { return this->usedVars_; }
};


#endif // STACKTRACE_H
