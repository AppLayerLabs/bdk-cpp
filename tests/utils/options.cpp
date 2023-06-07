#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/options.h"
#include <filesystem>


namespace TOptions {
  TEST_CASE("Option Class", "[core][options]") {
    SECTION("Options from File (default)") {
      Options optionsWithPrivKey(
        "optionClassFromFileWithPrivKey",
        "OrbiterSDK/cpp/linux_x86-64/0.0.3",
        1,
        8080,
        8080,
        8081,
        {},
        PrivKey(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66"))
      );

      Options optionsFromFileWithPrivKey(Options::fromFile("optionClassFromFileWithPrivKey"));

      REQUIRE(optionsFromFileWithPrivKey.getRootPath() == optionsWithPrivKey.getRootPath());
      REQUIRE(optionsFromFileWithPrivKey.getSDKVersion() == optionsWithPrivKey.getSDKVersion());
      REQUIRE(optionsFromFileWithPrivKey.getWeb3ClientVersion() == optionsWithPrivKey.getWeb3ClientVersion());
      REQUIRE(optionsFromFileWithPrivKey.getVersion() == optionsWithPrivKey.getVersion());
      REQUIRE(optionsFromFileWithPrivKey.getChainID() == optionsWithPrivKey.getChainID());
      REQUIRE(optionsFromFileWithPrivKey.getWsPort() == optionsWithPrivKey.getWsPort());
      REQUIRE(optionsFromFileWithPrivKey.getHttpPort() == optionsWithPrivKey.getHttpPort());
      REQUIRE(optionsFromFileWithPrivKey.getCoinbase() == optionsWithPrivKey.getCoinbase());
      REQUIRE(optionsFromFileWithPrivKey.getValidatorPrivKey() == optionsWithPrivKey.getValidatorPrivKey());
    }
  }
}