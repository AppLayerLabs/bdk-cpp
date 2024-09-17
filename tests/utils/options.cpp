/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/options.h"
#include <filesystem>

const std::vector<PrivKey> validatorPrivKeys_ {
  PrivKey(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
  PrivKey(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
  PrivKey(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
  PrivKey(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
  PrivKey(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
  PrivKey(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
  PrivKey(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
  PrivKey(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
};

namespace TOptions {
  TEST_CASE("Option Class", "[core][options]") {
    std::string testDumpPath = Utils::getTestDumpPath();
    SECTION("Options from File (default)") {
      if(std::filesystem::exists(testDumpPath + "/optionClassFromFileWithPrivKey")) {
        std::filesystem::remove_all(testDumpPath + "/optionClassFromFileWithPrivKey");
      }
      PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      uint64_t genesisTimestamp = 1678887538000000;
      FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{}, Hash(), genesisTimestamp, 0, genesisPrivKey);
      std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
      std::vector<Address> genesisValidators;
      for (const auto& privKey : validatorPrivKeys_) {
        genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
      }
      Options optionsWithPrivKey(
        testDumpPath + "/optionClassFromFileWithPrivKey",
        "BDK/cpp/linux_x86-64/0.2.0",
        1,
        8080,
        Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
        LOCALHOST,
        8080,
        8081,
        11,
        11,
        200,
        50,
        2000,
        10000,
        1000,
        4,
        {},
        genesis,
        genesisTimestamp,
        genesisPrivKey,
        genesisBalances,
        genesisValidators,
        PrivKey(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
        IndexingMode::RPC,
        json()
      );

      Options optionsFromFileWithPrivKey(Options::fromFile(testDumpPath + "/optionClassFromFileWithPrivKey"));
      REQUIRE(optionsFromFileWithPrivKey.getRootPath() == optionsWithPrivKey.getRootPath());
      REQUIRE(optionsFromFileWithPrivKey.getMajorSDKVersion() == optionsWithPrivKey.getMajorSDKVersion());
      REQUIRE(optionsFromFileWithPrivKey.getMinorSDKVersion() == optionsWithPrivKey.getMinorSDKVersion());
      REQUIRE(optionsFromFileWithPrivKey.getPatchSDKVersion() == optionsWithPrivKey.getPatchSDKVersion());
      REQUIRE(optionsFromFileWithPrivKey.getWeb3ClientVersion() == optionsWithPrivKey.getWeb3ClientVersion());
      REQUIRE(optionsFromFileWithPrivKey.getVersion() == optionsWithPrivKey.getVersion());
      REQUIRE(optionsFromFileWithPrivKey.getChainOwner() == optionsWithPrivKey.getChainOwner());
      REQUIRE(optionsFromFileWithPrivKey.getChainID() == optionsWithPrivKey.getChainID());
      REQUIRE(optionsFromFileWithPrivKey.getP2PIp() == optionsWithPrivKey.getP2PIp());
      REQUIRE(optionsFromFileWithPrivKey.getP2PPort() == optionsWithPrivKey.getP2PPort());
      REQUIRE(optionsFromFileWithPrivKey.getHttpPort() == optionsWithPrivKey.getHttpPort());
      REQUIRE(optionsFromFileWithPrivKey.getMinDiscoveryConns() == optionsWithPrivKey.getMinDiscoveryConns());
      REQUIRE(optionsFromFileWithPrivKey.getMinNormalConns() == optionsWithPrivKey.getMinNormalConns());
      REQUIRE(optionsFromFileWithPrivKey.getMaxDiscoveryConns() == optionsWithPrivKey.getMaxDiscoveryConns());
      REQUIRE(optionsFromFileWithPrivKey.getMaxNormalConns() == optionsWithPrivKey.getMaxNormalConns());
      REQUIRE(optionsFromFileWithPrivKey.getEventBlockCap() == optionsWithPrivKey.getEventBlockCap());
      REQUIRE(optionsFromFileWithPrivKey.getEventLogCap() == optionsWithPrivKey.getEventLogCap());
      REQUIRE(optionsFromFileWithPrivKey.getStateDumpTrigger() == optionsWithPrivKey.getStateDumpTrigger());
      REQUIRE(optionsFromFileWithPrivKey.getMinValidators() == optionsWithPrivKey.getMinValidators());
      REQUIRE(optionsFromFileWithPrivKey.getCoinbase() == optionsWithPrivKey.getCoinbase());
      REQUIRE(optionsFromFileWithPrivKey.getIsValidator() == optionsWithPrivKey.getIsValidator());
      REQUIRE(optionsFromFileWithPrivKey.getDiscoveryNodes() == optionsWithPrivKey.getDiscoveryNodes());
      REQUIRE(optionsFromFileWithPrivKey.getGenesisBlock() == optionsWithPrivKey.getGenesisBlock());
      REQUIRE(optionsFromFileWithPrivKey.getGenesisBalances() == optionsWithPrivKey.getGenesisBalances());
      REQUIRE(optionsFromFileWithPrivKey.getGenesisValidators() == optionsWithPrivKey.getGenesisValidators());

    }
  }
}
