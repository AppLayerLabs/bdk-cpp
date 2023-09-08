#ifndef ENUMERABLENFTATTRIBUTES_H
#define ENUMERABLENFTATTRIBUTES_H

#include "../../utils/utils.h";
#include "../variables/safeenumerableset.h";
#include "../variables/enumerableset.h";
#include "../variables/safeunorderedmap.h";
#include "../abi.h";
#include "../dynamiccontract.h";
#include "enumerableattributes.h";

#include <utility>

class EnumerableNftAttributes {
  public:
    const std::string E_NON_EXISTANT_KEY = "E_NEK";
    const std::string E_INDEX_OUT_OF_BOUND = "E_IOB";

    struct NftAttributesMap {
      SafeEnumerableSet<Bytes> keys;
      SafeUnorderedMap<std::string, AttributesMap> values;

      NftAttributesMap(DynamicContract* contract) : keys(contract), values(contract) {}

      AttributesMap getAttributesById(const std::string& key) {
        Bytes keyBytes(key.cbegin(), key.cend());
        if (!this->keys.contains(keyBytes)) throw std::runtime_error(E_NON_EXISTANT_KEY);
        return this->values[key];
      }

      std::pair<std::string, AttributesMap> getAttributesByIndex(uint256_t index) {
        return this->at(index);
      }

      std::vector<Attribute> getNftAttributeById(const std::string& uniqueId) {
        AttributesMap attributesMap = this->getAttributesById(uniqueId);
        uint256_t attributesSize = attributesMap.length();
        std::vector<Attribute> attributes;
        for (uint256_t i = 0; i < attributesSize; i++) {
          attributes.push_back(attributesMap.at(i));
        }
        return attributes;
      }

      std::pair<std::string, std::vector<Attribute>> getNftAttributeByIndex(uint256_t index) {
        std::pair<std::string, AttributesMap> p = this->getAttributesByIndex(index);
        uint256_t attributesSize = p.second.length();
        std::vector<Attribute> attributes;
        for (uint256_t i = 0; i < attributesSize; i++) {
          attributes.push_back(p.second.at(i));
        }
        return std::make_pair(uniqueId, attributes);
      }

      bool set(const std::string& key, const std::string& attributeKey, Attribute attributeValue) {
        Bytes keyBytes(key.cbegin(), key.cend());
        this->values[key].set(attributeKey, attributeValue);
        return this->keys.add(keyBytes);
      }

      // TODO: shouldn't this be also removing the key from keys? It's not doing it in the original
      bool remove(const std::string& key, const std::string& attributeKey) {
        return this->values[key].remove(attributeKey);
      }

      bool contains(const std::string& key) {
        Bytes keyBytes(key.cbegin(), key.cend());
        return this->keys.contains(keyBytes);
      }

      uint256_t length() { return this->keys.length(); }

      std::pair<std::string, AttributesMap> at(uint256_t index) {
        if (index >= this->keys.length()) throw std::runtime_error(E_INDEX_OUT_OF_BOUND);
        Bytes key = this->keys.at(index);
        std::string keyStr = Utils::bytesToString(key);
        return std::make_pair(keyStr, this->values[key]);
      }

      AttributesMap get(const std::string& key) {
        Bytes keyBytes(key.cbegin(), key.cend());
        AttributesMap value = this->values[key];
        if (value.length() == 0 && !this->keys.contains(keyBytes)) {
          throw std::runtime_error(E_NON_EXISTANT_KEY);
        }
        return value;
      }
    };
};

#endif  // ENUMERABLENFTATTRIBUTES_H
