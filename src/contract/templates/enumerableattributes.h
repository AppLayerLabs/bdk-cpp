#ifndef ENUMERABLEATTRIBUTES_H
#define ENUMERABLEATTRIBUTES_H

#include "../../utils/utils.h";
#include "../variables/safeenumerableset.h";
#include "../variables/safeunorderedmap.h";
#include "../abi.h";
#include "../dynamiccontract.h";

#include <utility>

class EnumerableAttributes {
  public:
    const std::string E_NON_EXISTANT_KEY = "E_NEK";

    struct Attribute {
      std::string name;
      std::string value;
    };

    struct AttributesMap {
      EnumerableSet<Bytes> keys;
      std::unordered_map<std::string, Attribute> values;

      bool set(const std::string& key, Attribute value) {
        Bytes keyBytes(key.cbegin(), key.cend());
        this->values[key] = value;
        return this->keys.add(keyBytes);
      }

      bool remove(const std::string& key) {
        Bytes keyBytes(key.cbegin(), key.cend());
        this->values.erase(key);
        return this->keys.remove(keyBytes);
      }

      bool contains(const std::string& key) {
        Bytes keyBytes(key.cbegin(), key.cend());
        return this->keys.contains(keyBytes);
      }

      uint256_t length() { return this->keys.length(); }

      // TODO: index must be less than length but this wasn't addressed in the original
      std::pair<std::string, Attribute> at(uint256_t index) {
        Bytes key = this->keys.at(index);
        return std::make_pair(Utils::bytesToString(key), this->values[key]);
      }

      Attribute get(const std::string& key) {
        Bytes keyBytes(key.cbegin(), key.cend());
        Attribute value = this->values[key];
        if (value.name.length() == 0 && !this->keys.contains(keyBytes)) {
          throw std::runtime_error(E_NON_EXISTANT_KEY);
        }
        return value;
      }
    };

    struct SafeAttributesMap {
      SafeEnumerableSet<Bytes> keys;
      SafeUnorderedMap<std::string, Attribute> values;

      SafeAttributesMap(DynamicContract* contract) : keys(contract), values(contract) {}

      bool set(const std::string& key, Attribute value) {
        Bytes keyBytes(key.cbegin(), key.cend());
        this->values[key] = value;
        return this->keys.add(keyBytes);
      }

      bool remove(const std::string& key) {
        Bytes keyBytes(key.cbegin(), key.cend());
        this->values.erase(key);
        return this->keys.remove(keyBytes);
      }

      bool contains(const std::string& key) {
        Bytes keyBytes(key.cbegin(), key.cend());
        return this->keys.contains(keyBytes);
      }

      uint256_t length() { return this->keys.length(); }

      // TODO: index must be less than length but this wasn't addressed in the original
      std::pair<std::string, Attribute> at(uint256_t index) {
        Bytes key = this->keys.at(index);
        return std::make_pair(Utils::bytesToString(key), this->values[key]);
      }

      Attribute get(const std::string& key) {
        Bytes keyBytes(key.cbegin(), key.cend());
        Attribute value = this->values[key];
        if (value.name.length() == 0 && !this->keys.contains(keyBytes)) {
          throw std::runtime_error(E_NON_EXISTANT_KEY);
        }
        return value;
      }
    };
};

#endif  // ENUMERABLEATTRIBUTES_H
