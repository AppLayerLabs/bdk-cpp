#ifndef ENUMERABLETREASURY_H
#define ENUMERABLETREASURY_H

#include "../../utils/utils.h";
#include "../variables/safeenumerableset.h";
#include "../variables/safeunorderedmap.h";
#include "../abi.h";
#include "../dynamiccontract.h";

#include <utility>

class EnumerableTreasury : ContractGlobals {
  public:
    const std::string E_NON_EXISTANT_KEY = "E_NEK";

    struct Transaction {
      uint256_t index;
      std::string action;
      uint256_t timestamp;
      uint256_t blockNumber;
      std::string symbol;
      uint256_t amount;
      Address from;
      Address to;
    };

    struct TransactionsMap {
      SafeEnumerableSet<uint256_t> keys;
      SafeUnorderedMap<uint256_t, Transaction> values;

      TransactionsMap(DynamicContract* contract) : keys(contract), values(contract) {}

      bool set(
        const std::string& action, const std::string& symbol,
        uint256_t amount, Address from, Address to
      ) {
        uint256_t index = this->keys.length();
        Transaction transaction = {
          index, action, this->getBlockTimestamp(), this->getBlockHeight(),
          symbol, amount, from, to
        }
        this->values[index] = transaction;
        return this->keys.add(index);
      }

      bool remove(uint256_t index) {
        this->values.erase(index);
        return this->keys.remove(index);
      }

      bool contains(uint256_t index) { return this->keys.contains(index); }

      uint256_t length() { return this->keys.length(); }

      // TODO: index must be less than length but this wasn't addressed in the original
      std::pair<uint256_t, Transaction> at(uint256_t key) {
        return std::make_pair(key, this->values[key]);
      }

      Transaction get(uint256_t key) {
        Transaction value = this->values[key];
        if (value.action.length() == 0 && !this->keys.contains(key)) {
          throw std::runtime_error(E_NON_EXISTANT_KEY);
        }
        return value;
      }
    };
};

#endif  // ENUMERABLETREASURY_H
