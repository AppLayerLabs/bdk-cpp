/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeunorderedmap.h"
#include <iostream>

namespace TSafeUnorderedMap {
  TEST_CASE("SafeUnorderedMap Class", "[contract][variables][safeunorderedmap]") {
    SECTION("SafeUnorderedMap Constructor") {
      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;
      auto randomAddress = Address(Utils::randBytes(20));
      safeUnorderedMap[randomAddress] = uint256_t("19283815712031512");
      REQUIRE(safeUnorderedMap.size() == 1);
      auto found = safeUnorderedMap.find(randomAddress);
      REQUIRE(found != safeUnorderedMap.end());
      REQUIRE(found->second == uint256_t("19283815712031512"));

      REQUIRE(safeUnorderedMap[randomAddress] == uint256_t("19283815712031512"));
      safeUnorderedMap.revert();
      REQUIRE(safeUnorderedMap.size() == 0);
      found = safeUnorderedMap.find(randomAddress);
      REQUIRE(found == safeUnorderedMap.end());
    }

    SECTION("SafeUnorderedMap Insert") {
      std::vector<Address> randomAddresses;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAddresses.emplace_back(Address(Utils::randBytes(20)));
      }

      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;

      for (const auto& address : randomAddresses) {
        safeUnorderedMap.insert({address, uint256_t("124918123712956236125812263412317341")});
      }

      SafeUnorderedMap safeUnorderedMapCopy = safeUnorderedMap;

      for (const auto& [address, balance] : safeUnorderedMap) {
        REQUIRE(balance == uint256_t("124918123712956236125812263412317341"));
      }
      safeUnorderedMap.commit();
      safeUnorderedMapCopy.revert();
      REQUIRE(safeUnorderedMap.size() == 100);
      for (const auto& address : randomAddresses) {
        auto found = safeUnorderedMap.find(address);
        REQUIRE(found != safeUnorderedMap.end());
      }
      REQUIRE(safeUnorderedMapCopy.size() == 0);
      for (const auto& address : randomAddresses) {
        auto found = safeUnorderedMapCopy.find(address);
        REQUIRE(found == safeUnorderedMapCopy.end());
      }
    }

    SECTION("SafeUnorderedMap insert_or_assign") {
      std::vector<Address> randomAddresses;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAddresses.emplace_back(Address(Utils::randBytes(20)));
      }

      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;

      for (const auto& address : randomAddresses) {
        safeUnorderedMap.insert_or_assign(address, uint256_t("124918123712956236125812263412317341"));
      }

      SafeUnorderedMap safeUnorderedMapCopy = safeUnorderedMap;

      for (const auto& [address, balance] : safeUnorderedMap) {
        REQUIRE(balance == uint256_t("124918123712956236125812263412317341"));
      }
      safeUnorderedMap.commit();
      safeUnorderedMapCopy.revert();
      REQUIRE(safeUnorderedMap.size() == 100);
      for (const auto& address : randomAddresses) {
        auto found = safeUnorderedMap.find(address);
        REQUIRE(found != safeUnorderedMap.end());
      }
      REQUIRE(safeUnorderedMapCopy.size() == 0);
      for (const auto& address : randomAddresses) {
        auto found = safeUnorderedMapCopy.find(address);
        REQUIRE(found == safeUnorderedMapCopy.end());
      }
    }

    SECTION("SafeUnorderedMap emplace") {
      std::vector<Address> randomAddresses;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAddresses.emplace_back(Address(Utils::randBytes(20)));
      }

      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;

      for (const auto& address : randomAddresses) {
        safeUnorderedMap.emplace(address, uint256_t("124918123712956236125812263412317341"));
      }

      SafeUnorderedMap safeUnorderedMapCopy = safeUnorderedMap;

      for (const auto& [address, balance] : safeUnorderedMap) {
        REQUIRE(balance == uint256_t("124918123712956236125812263412317341"));
      }
      safeUnorderedMap.commit();
      safeUnorderedMapCopy.revert();
      REQUIRE(safeUnorderedMap.size() == 100);
      for (const auto& address : randomAddresses) {
        auto found = safeUnorderedMap.find(address);
        REQUIRE(found != safeUnorderedMap.end());
      }
      REQUIRE(safeUnorderedMapCopy.size() == 0);
      for (const auto& address : randomAddresses) {
        auto found = safeUnorderedMapCopy.find(address);
        REQUIRE(found == safeUnorderedMapCopy.end());
      }
    }

    SECTION("SafeUnorderedMap erase") {
      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;
      auto randomAddress = Address(Utils::randBytes(20));
      safeUnorderedMap[randomAddress] = uint256_t("19283815712031512");
      REQUIRE(safeUnorderedMap.size() == 1);
      auto found = safeUnorderedMap.find(randomAddress);
      REQUIRE(found != safeUnorderedMap.end());
      REQUIRE(found->second == uint256_t("19283815712031512"));
      safeUnorderedMap.commit();
      REQUIRE(safeUnorderedMap.size() == 1);
      REQUIRE(safeUnorderedMap[randomAddress] == uint256_t("19283815712031512"));
      safeUnorderedMap.erase(randomAddress);
      safeUnorderedMap.commit();
      REQUIRE(safeUnorderedMap.size() == 0);
      found = safeUnorderedMap.find(randomAddress);
      REQUIRE(found == safeUnorderedMap.end());
    }

    SECTION("SafeUnorderedMap at") {
      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;
      auto randomAddress = Address(Utils::randBytes(20));
      safeUnorderedMap[randomAddress] = uint256_t("19283815712031512");
      REQUIRE(safeUnorderedMap.size() == 1);
      auto found = safeUnorderedMap.find(randomAddress);
      REQUIRE(found != safeUnorderedMap.end());
      REQUIRE(found->second == uint256_t("19283815712031512"));
      safeUnorderedMap.commit();
      REQUIRE(safeUnorderedMap.size() == 1);
      REQUIRE(safeUnorderedMap[randomAddress] == uint256_t("19283815712031512"));
      REQUIRE(safeUnorderedMap.at(randomAddress) == uint256_t("19283815712031512"));
      REQUIRE_THROWS(safeUnorderedMap.at(Address(Utils::randBytes(20))));
    }

    SECTION("SafeUnorderedMap operator[]") {
      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;
      auto randomAddress = Address(Utils::randBytes(20));
      safeUnorderedMap[randomAddress] = uint256_t("19283815712031512");
      REQUIRE(safeUnorderedMap.size() == 1);
      auto found = safeUnorderedMap.find(randomAddress);
      REQUIRE(found != safeUnorderedMap.end());
      REQUIRE(found->second == uint256_t("19283815712031512"));
      safeUnorderedMap.commit();
      REQUIRE(safeUnorderedMap.size() == 1);
      REQUIRE(safeUnorderedMap[randomAddress] == uint256_t("19283815712031512"));
    }

    SECTION("SafeUnorderedMap operator=") {
      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;
      auto randomAddress = Address(Utils::randBytes(20));
      safeUnorderedMap[randomAddress] = uint256_t("19283815712031512");
      REQUIRE(safeUnorderedMap.size() == 1);
      auto found = safeUnorderedMap.find(randomAddress);
      REQUIRE(found != safeUnorderedMap.end());
      REQUIRE(found->second == uint256_t("19283815712031512"));
      safeUnorderedMap.commit();
      SafeUnorderedMap<Address, uint256_t> safeUnorderedMapCopy;
      safeUnorderedMapCopy = safeUnorderedMap;
      safeUnorderedMapCopy.commit();
      REQUIRE(safeUnorderedMap.size() == 1);
      REQUIRE(safeUnorderedMap[randomAddress] == uint256_t("19283815712031512"));
      REQUIRE(safeUnorderedMapCopy.size() == 1);
      REQUIRE(safeUnorderedMapCopy[randomAddress] == uint256_t("19283815712031512"));
      auto randomAddress2 = Address(Utils::randBytes(20));
      REQUIRE(randomAddress != randomAddress2);
      safeUnorderedMap[randomAddress2] = uint256_t("11111111111111111");
      REQUIRE(safeUnorderedMap.size() == 2);
      auto it = safeUnorderedMap.find(randomAddress);
      safeUnorderedMap.erase(it);
      REQUIRE(safeUnorderedMap.size() == 1);
      safeUnorderedMapCopy = safeUnorderedMap;
      REQUIRE(safeUnorderedMapCopy.size() == 1);
      it = safeUnorderedMapCopy.find(randomAddress);
      REQUIRE(it == safeUnorderedMapCopy.end());
      it = safeUnorderedMapCopy.find(randomAddress2);
      REQUIRE(it != safeUnorderedMapCopy.end());
      REQUIRE(it->second == uint256_t("11111111111111111"));
      safeUnorderedMap[randomAddress] = uint256_t("19283815712031512");
      REQUIRE(safeUnorderedMap.size() == 2);
    }

    SECTION("SafeUnorderedMap erase-insert-commit regression") {
      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;
      auto randomAddress = Address(Utils::randBytes(20));
      safeUnorderedMap[randomAddress] = uint256_t("19283815712031512");
      safeUnorderedMap.commit();
      auto it = safeUnorderedMap.find(randomAddress);
      if (it == safeUnorderedMap.end()) {
        return;
      }
      safeUnorderedMap.erase(it);
      safeUnorderedMap[randomAddress] = uint256_t("19283815712031512");
      safeUnorderedMap.commit();
      REQUIRE(safeUnorderedMap[randomAddress] == uint256_t("19283815712031512"));
    }

    SECTION("SafeUnorderedMap count") {
      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;
      auto randomAddress = Address(Utils::randBytes(20));
      safeUnorderedMap[randomAddress] = uint256_t("19283815712031512");
      REQUIRE(safeUnorderedMap.size() == 1);
      auto found = safeUnorderedMap.find(randomAddress);
      REQUIRE(found != safeUnorderedMap.end());
      REQUIRE(found->second == uint256_t("19283815712031512"));
      safeUnorderedMap.commit();
      REQUIRE(safeUnorderedMap.size() == 1);
      REQUIRE(safeUnorderedMap[randomAddress] == uint256_t("19283815712031512"));
      REQUIRE(safeUnorderedMap.count(randomAddress) == 1);
      REQUIRE(safeUnorderedMap.count(Address(Utils::randBytes(20))) == 0);
    }

    SECTION("SafeUnorderedMap find") {
      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;
      auto randomAddress = Address(Utils::randBytes(20));
      safeUnorderedMap[randomAddress] = uint256_t("19285123125124152");
      REQUIRE(safeUnorderedMap.size() == 1);
      auto found = safeUnorderedMap.find(randomAddress);
      REQUIRE(found != safeUnorderedMap.end());
      REQUIRE(found->second == uint256_t("19285123125124152"));
      found->second = uint256_t("64512342624123513");
      safeUnorderedMap.commit();
      REQUIRE(safeUnorderedMap.size() == 1);
      REQUIRE(safeUnorderedMap[randomAddress] == uint256_t("64512342624123513"));
      REQUIRE(safeUnorderedMap.count(randomAddress) == 1);
      REQUIRE(safeUnorderedMap.count(Address(Utils::randBytes(20))) == 0);
    }

    SECTION("SafeUnorderedMap contains") {
      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;
      auto randomAddress = Address(Utils::randBytes(20));
      safeUnorderedMap[randomAddress] = uint256_t("19283815712031512");
      REQUIRE(safeUnorderedMap.size() == 1);
      auto found = safeUnorderedMap.find(randomAddress);
      REQUIRE(found != safeUnorderedMap.end());
      REQUIRE(found->second == uint256_t("19283815712031512"));
      safeUnorderedMap.commit();
      REQUIRE(safeUnorderedMap.size() == 1);
      REQUIRE(safeUnorderedMap[randomAddress] == uint256_t("19283815712031512"));
      REQUIRE(safeUnorderedMap.contains(randomAddress));
      REQUIRE(!safeUnorderedMap.contains(Address(Utils::randBytes(20))));
    }

    SECTION("SafeUnorderedMap custom iterator loop modify") {
      SafeUnorderedMap<uint64_t, uint64_t> map;
      map[0] = 1;
      map[1] = 2;
      map[2] = 3;
      map[3] = 4;
      map[4] = 5;
      map.commit();
      REQUIRE(map.size() == 5);
      map.erase(2);
      REQUIRE(map.size() == 4);
      map[2] = 33;
      map[3] = 44;
      REQUIRE(map.size() == 5);
      std::vector<uint64_t> seenModify(5, 0);
      auto it = map.begin();
      while (it != map.end()) {
        auto it2 = it;
        REQUIRE(it2 == it);
        it->second += 1000;
        REQUIRE(it2 == it);
        ++seenModify[it->first];
        ++it;
      }
      REQUIRE(map[0] == 1001);
      REQUIRE(map[1] == 1002);
      REQUIRE(map[2] == 1033);
      REQUIRE(map[3] == 1044);
      REQUIRE(map[4] == 1005);
      REQUIRE(seenModify[0] == 1);
      REQUIRE(seenModify[1] == 1);
      REQUIRE(seenModify[2] == 1);
      REQUIRE(seenModify[3] == 1);
      REQUIRE(seenModify[4] == 1);
      std::vector<uint64_t> seenErase(5, 0);
      it = map.begin();
      int targetSize = 5;
      while (it != map.end()) {
        ++seenErase[it->first];
        it = map.erase(it);
        --targetSize;
        REQUIRE(map.size() == targetSize);
      }
      REQUIRE(seenErase[0] == 1);
      REQUIRE(seenErase[1] == 1);
      REQUIRE(seenErase[2] == 1);
      REQUIRE(seenErase[3] == 1);
      REQUIRE(seenErase[4] == 1);
      SafeUnorderedMap<uint64_t, uint64_t> map2;
      map2.reserve(1000);
      map2[0] = 1;
      map2[2] = 3;
      map2[4] = 5;
      map2[6] = 7;
      map2[8] = 9;
      REQUIRE(map2.size() == 5);
      std::vector<uint64_t> seenInsert(10, 0);
      it = map2.begin();
      targetSize = 5;
      while (it != map2.end()) {
        ++seenInsert[it->first];
        if (it->first % 2 == 0) {
          uint64_t keyToInsert = 9 - it->first;
          REQUIRE(map2.find(keyToInsert) == map2.end());
          map2.insert({keyToInsert, it->first});
          ++targetSize;
          REQUIRE(map2.size() == targetSize);
        }
        ++it;
      }
      REQUIRE(seenInsert[0] <= 1);
      REQUIRE(seenInsert[1] <= 1);
      REQUIRE(seenInsert[2] <= 1);
      REQUIRE(seenInsert[3] <= 1);
      REQUIRE(seenInsert[4] <= 1);
      REQUIRE(seenInsert[5] <= 1);
      REQUIRE(seenInsert[6] <= 1);
      REQUIRE(seenInsert[7] <= 1);
      REQUIRE(seenInsert[8] <= 1);
      REQUIRE(seenInsert[9] <= 1);
      REQUIRE(map2.size() == 10);
      SafeUnorderedMap<uint64_t, uint64_t> map3;
      for (int i=0; i<10; ++i) {
        map3[i] = i;
        if (i == 6) map3.commit();
      }
      for (const auto& [key, value] : map3) {
        REQUIRE(key == value);
      }
    }

    SECTION("SafeUnorderedMap erase with forwarding") {
      SafeUnorderedMap<std::string, int> safeUnorderedMap;
      const char* key1 = "key1";
      const char* key2 = "key2";
      safeUnorderedMap[key1] = 10;
      safeUnorderedMap.commit();
      safeUnorderedMap[key2] = 20;
      REQUIRE(safeUnorderedMap.size() == 2);
      REQUIRE(safeUnorderedMap.count(key1) == 1);
      REQUIRE(safeUnorderedMap.count(key2) == 1);
      auto erasedCount1 = safeUnorderedMap.erase(std::move(key1));
      REQUIRE(erasedCount1 == 1);
      REQUIRE(safeUnorderedMap.size() == 1);
      REQUIRE(safeUnorderedMap.count(key1) == 0);
      auto erasedCount2 = safeUnorderedMap.erase(std::string(key2));
      REQUIRE(erasedCount2 == 1);
      REQUIRE(safeUnorderedMap.size() == 0);
      REQUIRE(safeUnorderedMap.count(key2) == 0);
    }

    SECTION("SafeUnorderedMap insert_or_assign with move") {
      std::vector<Address> randomAddresses;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAddresses.emplace_back(Address(Utils::randBytes(20)));
      }

      SafeUnorderedMap<Address, uint256_t> safeUnorderedMap;
      REQUIRE(safeUnorderedMap.size() == 0);
      for (auto& address : randomAddresses) {
        safeUnorderedMap.insert_or_assign(std::move(address), uint256_t("124918123712956236125812263412317341"));
      }
      REQUIRE(safeUnorderedMap.size() == 100);

      SafeUnorderedMap safeUnorderedMapCopy = safeUnorderedMap;

      for (const auto& [address, balance] : safeUnorderedMap) {
        REQUIRE(balance == uint256_t("124918123712956236125812263412317341"));
      }
      safeUnorderedMap.commit();
      safeUnorderedMapCopy.revert();
      REQUIRE(safeUnorderedMap.size() == 100);
      for (const auto& address : randomAddresses) {
        auto found = safeUnorderedMap.find(address);
        REQUIRE(found != safeUnorderedMap.end());
      }
      REQUIRE(safeUnorderedMapCopy.size() == 0);
      for (const auto& address : randomAddresses) {
        auto found = safeUnorderedMapCopy.find(address);
        REQUIRE(found == safeUnorderedMapCopy.end());
      }
    }

    SECTION("SafeUnorderedMap hint") {
      {
        SafeUnorderedMap<uint64_t, uint64_t> map;
        map.emplace_hint(map.cend(), 0, 1);
        REQUIRE(map[0] == 1);
      }
      {
        SafeUnorderedMap<uint64_t, uint64_t> map;
        map[0] = 0;
        auto hint = std::as_const(map).find(0);
        std::pair<const uint64_t, uint64_t> keyValue(0, 1);
        auto it = map.insert(hint, keyValue);
        REQUIRE(map[0] == 0);
        REQUIRE(it == hint);
      }
      {
        SafeUnorderedMap<uint64_t, uint64_t> map;
        map[0] = 0;
        auto hint = std::as_const(map).find(0);
        auto it = map.insert(hint, std::make_pair(0, 1));
        REQUIRE(map[0] == 0);
        REQUIRE(it == hint);
      }
      {
        SafeUnorderedMap<uint64_t, uint64_t> map;
        map[0] = 0;
        auto hint = std::as_const(map).find(0);
        uint64_t key = 0;
        uint64_t value = 1;
        auto it = map.insert_or_assign(hint, key, value);
        REQUIRE(map[0] == 1);
        REQUIRE(it == hint);
      }
      {
        SafeUnorderedMap<uint64_t, uint64_t> map;
        map[0] = 0;
        auto hint = std::as_const(map).find(0);
        uint64_t key = 0;
        uint64_t value = 1;
        auto it = map.insert_or_assign(hint, std::move(key), std::move(value));
        REQUIRE(map[0] == 1);
        REQUIRE(it == hint);
      }
    }

    SECTION("SafeUnorderedMap insert") {
      {
        SafeUnorderedMap<uint64_t, uint64_t> map;
        map[0] = 0;
        std::pair<const uint64_t, uint64_t> keyValue(0, 1);
        map.insert(keyValue);
        REQUIRE(map[0] == 0);
        REQUIRE(map.size() == 1);
      }
      {
        SafeUnorderedMap<uint64_t, uint64_t> map;
        map[0] = 0;
        map.insert(std::make_pair(0,1));
        REQUIRE(map[0] == 0);
        REQUIRE(map.size() == 1);
      }
    }
  }
}
