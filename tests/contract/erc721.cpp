/*
  Copyright (c) [2023-2024] [Sparq Network]
  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc721test.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/core/rdpos.h"
#include "../sdktestsuite.hpp"


namespace TERC721 {
  TEST_CASE("ERC721 Class", "[contract][erc721]") {
    SECTION("ERC721 Creation") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC721Creation");
      auto ERC721Address = sdk.deployContract<ERC721Test>(std::string("My Test NFT!"), std::string("NFT"), uint64_t(100));
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::name) == "My Test NFT!");
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::symbol) == "NFT");
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::maxTokens) == 100);
    }

    SECTION("ERC721 Mint 100 Token Same Address") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC721Mint100TokenSameAddress");
      auto ERC721Address = sdk.deployContract<ERC721Test>(std::string("My Test NFT!"), std::string("NFT"), uint64_t(100));
      for (uint64_t i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, &ERC721Test::mint, sdk.getChainOwnerAccount().address);
      }
      // Check if token Id 0...99 is minted and owned by the chain owner
      for (uint64_t i = 0; i < 100; ++i) {
        auto owner = sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(i));
        REQUIRE(owner == sdk.getChainOwnerAccount().address);
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, sdk.getChainOwnerAccount().address) == 100);
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 100);
    }

    SECTION("ERC721 Mint 100 Different Addresses") {
      // Generate 100 different TestAccounts
      std::vector<TestAccount> accounts;
      for (int i = 0; i < 100; ++i) {
        accounts.emplace_back(TestAccount::newRandomAccount());
      }
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC721Mint100DifferentAddresses", accounts);
      auto ERC721Address = sdk.deployContract<ERC721Test>(std::string("My Test NFT!"), std::string("NFT"), uint64_t(100));
      for (int i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, &ERC721Test::mint, accounts[i].address);
      }
      // Check if token Id 0...99 is minted and owned by each respective account
      for (int i = 0; i < 100; ++i) {
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(i)) == accounts[i].address);
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, accounts[i].address) == 1);
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 100);
    }

    SECTION("ERC721 Mint 100 Different Addresses reverse") {
      // Same as before, but we mint from the test accounts list in reverse order
      std::vector<TestAccount> accounts;
      for (int i = 0; i < 100; ++i) {
        accounts.emplace_back(TestAccount::newRandomAccount());
      }
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC721Mint100DifferentAddressesReverse", accounts);
      auto ERC721Address = sdk.deployContract<ERC721Test>(std::string("My Test NFT!"), std::string("NFT"), uint64_t(100));
      for (int i = 99; i >= 0; i--) {
        sdk.callFunction(ERC721Address, &ERC721Test::mint, accounts[i].address);
      }
      // Check if token Id 0...99 is minted and owned by each respective account
      uint64_t tokenId = 0;
      for (int i = 99; i >= 0; i--) {
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(tokenId++)) == accounts[i].address);
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, accounts[i].address) == 1);
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 100);
    }

    SECTION("ERC721 Mint 100 and Burn 100 Same Address") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC721Mint100AndBurn100SameAddress");
      auto ERC721Address = sdk.deployContract<ERC721Test>(std::string("My Test NFT!"), std::string("NFT"), uint64_t(100));
      for (uint64_t i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, &ERC721Test::mint, sdk.getChainOwnerAccount().address);
      }
      // Check if token Id 0...99 is minted and owned by the chain owner
      for (uint64_t i = 0; i < 100; ++i) {
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(i)) == sdk.getChainOwnerAccount().address);
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, sdk.getChainOwnerAccount().address) == 100);
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 100);
      for (uint64_t i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, &ERC721Test::burn, uint256_t(i));
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 0);
      // Make sure that ownerOf throws (token does not exist)
      for (uint64_t i = 0; i < 100; ++i) {
        REQUIRE_THROWS(sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(i)));
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, sdk.getChainOwnerAccount().address) == 0);
    }

    SECTION("ERC721 Mint 100 Different Address Burn 100 Different Address") {
      // Generate 100 different TestAccounts
      std::vector<TestAccount> accounts;
      for (int i = 0; i < 100; ++i) {
        accounts.emplace_back(TestAccount::newRandomAccount());
      }
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC721Mint100DifferentAddressBurn100DifferentAddress", accounts);
      auto ERC721Address = sdk.deployContract<ERC721Test>(std::string("My Test NFT!"), std::string("NFT"), uint64_t(100));
      for (int i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, &ERC721Test::mint, accounts[i].address);
      }
      // Check if token Id 0...99 is minted and owned by each respective account
      for (int i = 0; i < 100; ++i) {
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(i)) == accounts[i].address);
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, accounts[i].address) == 1);
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 100);
      for (int i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, accounts[i], &ERC721Test::burn, uint256_t(i));
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 0);
      // Make sure that ownerOf throws (token does not exist)
      for (uint64_t i = 0; i < 100; ++i) {
        REQUIRE_THROWS(sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(i)));
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, accounts[i].address) == 0);
      }
    }

    SECTION("ERC721 Mint 100 Different Address Burn With Allowance") {
      // Generate 100 different TestAccounts
      std::vector<TestAccount> accounts;
      for (int i = 0; i < 100; ++i) {
        accounts.emplace_back(TestAccount::newRandomAccount());
      }
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC721Mint100DifferentAddressBurnWithAllowance", accounts);
      auto ERC721Address = sdk.deployContract<ERC721Test>(std::string("My Test NFT!"), std::string("NFT"), uint64_t(100));
      for (int i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, &ERC721Test::mint, accounts[i].address);
      }
      // Check if token Id 0...99 is minted and owned by each respective account
      for (int i = 0; i < 100; ++i) {
        auto owner = sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(i));
        REQUIRE(owner == accounts[i].address);
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, accounts[i].address) == 1);
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 100);
      // REQUIRE THROW trying to burn without allowance
      for (int i = 0; i < 100; ++i) {
        REQUIRE_THROWS(sdk.callFunction(ERC721Address, &ERC721Test::burn, uint256_t(i)));
      }
      // REQUIRE THROW trying to authorize using an account that is not the owner
      for (int i = 0; i < 100; ++i) {
        REQUIRE_THROWS(sdk.callFunction(ERC721Address, &ERC721Test::approve, sdk.getChainOwnerAccount().address, uint256_t(i)));
      }
      for (int i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, accounts[i], &ERC721Test::approve, sdk.getChainOwnerAccount().address, uint256_t(i));
      }
      // Check allowance
      for (int i = 0; i < 100; ++i) {
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::getApproved, uint256_t(i)) == sdk.getChainOwnerAccount().address);
      }
      for (int i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, &ERC721Test::burn, uint256_t(i));
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 0);
      // Make sure that ownerOf throws (token does not exist)
      for (uint64_t i = 0; i < 100; ++i) {
        REQUIRE_THROWS(sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(i)));
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, accounts[i].address) == 0);
      }
      // Make sure allowance doesn't exists anymore
      for (int i = 0; i < 100; ++i) {
        REQUIRE_THROWS(sdk.callViewFunction(ERC721Address, &ERC721Test::getApproved, uint256_t(i)) == Address());
      }
    }

    SECTION("ERC721 transferFrom with allowance from 100 different accounts") {
      // Generate 100 different TestAccounts
      std::vector<TestAccount> accounts;
      for (int i = 0; i < 100; ++i) {
        accounts.emplace_back(TestAccount::newRandomAccount());
      }
      TestAccount accountToSent = TestAccount::newRandomAccount();
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC721Mint100DifferentAddressBurnWithAllowance", accounts);
      auto ERC721Address = sdk.deployContract<ERC721Test>(std::string("My Test NFT!"), std::string("NFT"), uint64_t(100));
      for (int i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, &ERC721Test::mint, accounts[i].address);
      }
      // Check if token Id 0...99 is minted and owned by each respective account
      for (int i = 0; i < 100; ++i) {
        auto owner = sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(i));
        REQUIRE(owner == accounts[i].address);
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, accounts[i].address) == 1);
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 100);
      // REQUIRE THROW for transferFrom without allowance
      for (int i = 0; i < 100; ++i) {
        REQUIRE_THROWS(sdk.callFunction(ERC721Address, &ERC721Test::transferFrom, accounts[i].address, accountToSent.address, uint256_t(i)));
      }
      // Give allowance
      for (int i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, accounts[i], &ERC721Test::approve, sdk.getChainOwnerAccount().address, uint256_t(i));
      }
      // Check allowance
      for (int i = 0; i < 100; ++i) {
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::getApproved, uint256_t(i)) == sdk.getChainOwnerAccount().address);
      }
      // Transfer from
      for (int i = 0; i < 100; ++i) {
        sdk.callFunction(ERC721Address, &ERC721Test::transferFrom, accounts[i].address, accountToSent.address, uint256_t(i));
      }
      // Check if token id 0...99 is owned by accountToSent
      for (int i = 0; i < 100; ++i) {
        auto owner = sdk.callViewFunction(ERC721Address, &ERC721Test::ownerOf, uint256_t(i));
        REQUIRE(owner == accountToSent.address);
        REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::balanceOf, accountToSent.address) == 100);
      }
      REQUIRE(sdk.callViewFunction(ERC721Address, &ERC721Test::totalSupply) == 100);
      // Make sure that the allowance was removed after the transfer
      for (int i = 0; i < 100; ++i) {
        REQUIRE_THROWS(sdk.callViewFunction(ERC721Address, &ERC721Test::getApproved, uint256_t(i)) == Address());
      }
    }
  }
}