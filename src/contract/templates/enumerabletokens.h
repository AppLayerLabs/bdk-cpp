#ifndef ENUMERABLETOKENS_H
#define ENUMERABLETOKENS_H

#include "../../utils/utils.h";
#include "../variables/safeenumerableset.h";
#include "../variables/safeunorderedmap.h";
#include "../abi.h";
#include "../dynamiccontract.h";

#include <utility>

class EnumerableTokens {
  private:
    Hash _toBytes32(const std::string& key) {
      Bytes keyBytes(key.cbegin(), key.cend());
      Utils::padRightBytes(keyBytes, 32);
      return Hash(keyBytes);
    }

  public:
    const std::string E_NON_EXISTANT_KEY = "E_NEK";

    struct Token {
      Address erc20;
      std::string name;
      std::string symbol;
      uint256_t index;
    };

    struct TokensMap {
      SafeEnumerableSet<Hash> keys;
      SafeUnorderedMap<Hash, Token> values;

      TokensMap(DynamicContract* contract) : keys(contracts), values(contract) {}

      std::vector<Token> getTokens() {
        std::vector<Token> tokens;
        for (uint256_t i = 0; i < this->values.size(); i++) {
          tokens.push_back(this->values.at(i));
        }
        return tokens;
      }

      bool set(const std::string& key, Token value) {
        Hash key32 = this->_toBytes32(key);
        this->values[key32] = value;
        return this->keys.add(key32);
      }

      bool remove(const std::string& key) {
        Hash key32 = this->_toBytes32(key);
        this->values.erase(key32);
        return this->keys.remove(key32);
      }

      bool contains(const std::string& key) {
        Hash key32 = this->_toBytes32(key);
        return this->keys.contains(key32);
      }

      uint256_t length() { return this->keys.length(); }

      std::pair<std::string, Token> at(uint256_t index) {
        Hash key = this->keys.at(index);
        Hash key32 = this->_toBytes32(key);
        return std::make_pair(key.hex().get(), this->values[key32]);
      }

      Token get(const std::string& key) {
        Hash key32 = this->_toBytes32(key);
        Token value = this->values[key32];
        if (value.symbol.size() == 0 && !this->keys.contains(key)) {
          throw std::runtime_error(E_NON_EXISTANT_KEY);
        }
        return value;
      }
    }
};

#endif  // ENUMERABLETOKENS_H
