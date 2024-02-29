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
  }
}
