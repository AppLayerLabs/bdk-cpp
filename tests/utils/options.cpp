/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/options.h"
#include <filesystem>


namespace TOptions {
  TEST_CASE("Option Class", "[core][options]") {
    std::string testDumpPath = Utils::getTestDumpPath();
    SECTION("Options from File (default)") {
      if(std::filesystem::exists(testDumpPath + "/optionClassFromFileWithPrivKey")) {
        std::filesystem::remove_all(testDumpPath + "/optionClassFromFileWithPrivKey");
      }
      PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      uint64_t genesisTimestamp = 1678887538000000;
      Block genesis(Hash(), 0, 0);
      genesis.finalize(genesisPrivKey, genesisTimestamp);
      std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
      Options optionsWithPrivKey(
        testDumpPath + "/optionClassFromFileWithPrivKey",
        "OrbiterSDK/cpp/linux_x86-64/0.1.2",
        1,
        8080,
        Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
        8080,
        8081,
        {},
        genesis,
        genesisTimestamp,
        genesisPrivKey,
        genesisBalances,
        PrivKey(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66"))
      );

      Options optionsFromFileWithPrivKey(Options::fromFile(testDumpPath + "/optionClassFromFileWithPrivKey"));

      REQUIRE(optionsFromFileWithPrivKey.getRootPath() == optionsWithPrivKey.getRootPath());
      REQUIRE(optionsFromFileWithPrivKey.getSDKVersion() == optionsWithPrivKey.getSDKVersion());
      REQUIRE(optionsFromFileWithPrivKey.getWeb3ClientVersion() == optionsWithPrivKey.getWeb3ClientVersion());
      REQUIRE(optionsFromFileWithPrivKey.getVersion() == optionsWithPrivKey.getVersion());
      REQUIRE(optionsFromFileWithPrivKey.getChainOwner() == optionsWithPrivKey.getChainOwner());
      REQUIRE(optionsFromFileWithPrivKey.getChainID() == optionsWithPrivKey.getChainID());
      REQUIRE(optionsFromFileWithPrivKey.getP2PPort() == optionsWithPrivKey.getP2PPort());
      REQUIRE(optionsFromFileWithPrivKey.getHttpPort() == optionsWithPrivKey.getHttpPort());
      REQUIRE(optionsFromFileWithPrivKey.getCoinbase() == optionsWithPrivKey.getCoinbase());
      REQUIRE(optionsFromFileWithPrivKey.getValidatorPrivKey() == optionsWithPrivKey.getValidatorPrivKey());
      REQUIRE(optionsFromFileWithPrivKey.getGenesisBlock() == optionsWithPrivKey.getGenesisBlock());
      REQUIRE(optionsFromFileWithPrivKey.getGenesisBalances() == optionsWithPrivKey.getGenesisBalances());
    }
  }
}